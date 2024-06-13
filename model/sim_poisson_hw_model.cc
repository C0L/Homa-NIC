#include <algorithm>
#include <iostream>
#include <list>
#include <queue>
#include <set>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <getopt.h>

#include <cstdint>
#include <cmath>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <limits>

#include <random>
#include <iostream>
#include <cassert>

#include "dist.h"

using namespace std;

struct simstat_t {
    uint64_t highwater; // # of times highwater mark triggered
    uint64_t lowwater;  // # of times lowwater mark triggered
    uint64_t max;       // Maximum occupancy of the queue
    uint64_t events;    // # of events (push/pop)
    uint64_t compcount; // # of completions
    uint64_t compsum;   // Some of MCTs
    uint64_t cycles;    // Total number of simulation cycles
    uint64_t qo;        // Sum queue Occupancy
    uint64_t inacc;     // Inaccuracies
};

struct entry_t {
    uint64_t ts;   // Start time
    int length;    // Original Length
    int remaining; // Remaining length
};


std::ostream& operator<<(std::ostream& os, const entry_t& entry) {
    return os << entry.ts << " " << entry.length << " " << entry.remaining;
}

// bool operator==(entry_t const& lhs, entry_t const& rhs)
// {
//     return (lhs.ts == rhs.ts && lhs.length == rhs.length && lhs.remaining == rhs.remaining);
// }


// Return false reflexively if they are equal
struct Compare {
    bool operator()(const entry_t & l, const entry_t & r) const {
	if (l.remaining < r.remaining) {
	    return true;
	} else if (l.remaining == r.remaining && l.ts < r.ts) {
	    return true;
	} else if (l.remaining == r.remaining && l.ts == r.ts && l.length < r.length) {
	    return true;
	}
	return false;
    }

};


static simstat_t simstat; 

const struct option longOpts[] {
    {"workload",required_argument,NULL,'w'},
    {"comps",required_argument,NULL,'c'},
    {"utilization",required_argument,NULL,'u'},
    {"trace-file",required_argument,NULL,'t'},
};

int main(int argc, char ** argv) {
    int c;

    opterr = 0;

    uint64_t comps;
    double utilization;
    char * end;
    std::string wfile;
    std::string tfile;

    while ((c = getopt_long(argc, argv, "w:u:c:t:", longOpts, NULL)) != -1)
	switch (c) {
	    case 'w':
		wfile = std::string(optarg);
		break;
	    case 'u':
		utilization = strtod(optarg, &end);
		break;
	    case 'c':
		comps = strtol(optarg, &end, 10);
		break;
	    case 't':
		tfile = std::string(optarg);
		break;
	    default:
		abort();
	}

    std::cerr << "Launching Simulator:" << std::endl;
    std::cerr << "  - Workload      : " << wfile << std::endl;
    std::cerr << "  - Utilization   : " << utilization << std::endl;
    std::cerr << "  - Completions   : " << comps << std::endl;
    std::cerr << "  - Trace File    : " << tfile << std::endl;

    std::mt19937 gen{0xdeadbeef}; 

    // Message distribution capped at 1MB
    dist_point_gen generator(wfile.c_str(), 1000000);

    // Compute the desired arrival cycles 
    double desired_arrival = generator.get_mean() * (1.0/utilization);

    // Interarrival process with desired mean
    std::poisson_distribution<int> arrivals(desired_arrival);

    // Initialize arrival and length
    uint64_t arrival = arrivals(gen);
    int length = generator(gen);

    // Golden queue 
    std::multiset<entry_t, Compare> gqueue;

    // Simulation queue
    std::multiset<entry_t, Compare> hwqueue;

    std::vector<entry_t> backing;

    // Simulation time
    uint64_t ts = 0;

    uint32_t min_index = 0;
    uint32_t search = 0;

    // Iterate through events until we reach the requisite number of completitions
    for (;simstat.compcount < comps;) {

	uint64_t buff = 64;
	// If the small queue is not empty, decrement the head or pop it
	while (!hwqueue.empty() && buff != 0) {
	    entry_t head = *hwqueue.begin();
	    hwqueue.erase(hwqueue.begin());

	    if (head.ts != (*gqueue.begin()).ts) {
		simstat.inacc++;
	    }

	    gqueue.erase(head);

	    head.remaining -= buff;

	    if (head.remaining > 0) {
		hwqueue.insert(head);
		gqueue.insert(head);
		buff = 0;
	    } else {
			
		buff -= (buff + head.remaining);

		simstat.events++;
		simstat.compsum += (ts - head.ts);
		simstat.compcount++;

		// head.remaining = head.length;

		// std::cerr << "Trying to remove " << head << std::endl;

		// std::cerr << "hwqueue" << std::endl;
		// for (auto & it : hwqueue) {
		//     std::cerr << it << std::endl;
		// }

		// std::cerr << "Gqueue" << std::endl;
		// for (auto & it : gqueue) {
		//     std::cerr << it << std::endl;
		// }

		// auto it = gqueue.find(head);
		// assert(it != gqueue.end());
		// std::cerr << "Trying to remove " << head.remaining << " " << head.length << " " << head.ts << std::endl;
		// std::cerr << "Found            " << (*it).remaining << " " << (*it).length << " " << (*it).ts << std::endl;

		// int pre = gqueue.size();
		// gqueue.erase(head);
		// int post = gqueue.size();

		// assert(post<pre);
	    }
	}

	// If the next arrival time is before or during this current timestep then add it to the queue
	while (std::floor(arrival/64.0) <= ts) {
	    entry_t ins = entry_t{ts, length, length};

	    assert(length != 0 && "initial length should not be 0");
	    hwqueue.insert(ins);
	    gqueue.insert(ins);

	    simstat.events++;

	    // Move to the next arrival and length
	    arrival += arrivals(gen);
	    length  = generator(gen);
	}

	// Two lists

	// Start reading through one, and write to the other one as you go,
	// Always withhold the smallest value
	// When you find a new smallest value, write your previous one to list 2 instead of value just read

	// If there are too few entries in small queue start a new search
	if (hwqueue.size() <= 2 && !backing.empty()) {
	    // Either iterate search, or reset it
	    entry_t candidate = backing[search];

	    if (candidate.remaining <= backing[min_index].remaining) {
		min_index = search;
	    }

	    search++;

	    // We reached the end, return the best we found
	    if (search == backing.size()) {
		simstat.lowwater++;
		entry_t best = backing[min_index];
		backing.erase(std::next(backing.begin(), min_index));
		hwqueue.insert(best);
		search = 0;
		min_index = 0;
	    }
	}

	// If there are too many entries in small queue, move them up
	if (hwqueue.size() >= 4) {
	    simstat.highwater++;

	    entry_t tail = *std::prev(hwqueue.end());
	    hwqueue.erase(std::prev(hwqueue.end()));

	    backing.push_back(tail);
	}

	// Move the timestep to the next event
	ts += 1;
    }

    simstat.cycles = ts;

    std::cerr << "Total Cmpl: " << simstat.compcount << std::endl;
    std::cerr << "Mean Cmpl: " << ((double) simstat.compsum) / simstat.compcount << std::endl;
    std::cerr << "Highwaters: " << ((double) simstat.highwater)/simstat.cycles << std::endl;
    std::cerr << "Lowwaters: " << ((double) simstat.lowwater)/simstat.cycles << std::endl;
    std::cerr << "Final Size: " << hwqueue.size() << std::endl;
    std::cerr << "Average Size: " << simstat.qo / simstat.cycles << std::endl;
    std::cerr << "Max Size: " << simstat.max << std::endl;
    std::cerr << "Inaccuracies: " << ((double) simstat.inacc) / simstat.cycles << std::endl;

    // for (int i = 0; i < 64; i++) {
    // 	std::cerr << i << " VALID COMPS " << slotstats[i].validcycles << " " << slotstats[i].backlog <<std::endl;
    // }

    string slotstatroot = tfile;
    int fslotstat = open(slotstatroot.append(".slotstats").c_str(), O_RDWR | O_CREAT, 0644);
    write(fslotstat, &simstat, sizeof(simstat_t));
    close(fslotstat);
}
