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
    uint64_t comps; // # of completions
    uint64_t sum_comps;   // Some of MCTs
    uint64_t cycles;    // Total number of simulation cycles
    uint64_t inacc;     // Inaccuracies
    uint64_t sum_slow_down;
};

struct entry_t {
    uint64_t ts;   // Start time
    int length;    // Original Length
    int remaining; // Remaining length
};

std::ostream& operator<<(std::ostream& os, const entry_t& entry) {
    return os << entry.ts << " " << entry.length << " " << entry.remaining;
}

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
    {"priorities",required_argument,NULL,'p'},
    {"utilization",required_argument,NULL,'u'},
    {"trace-file",required_argument,NULL,'t'},
};

int main(int argc, char ** argv) {
    int c;

    opterr = 0;

    uint64_t comps;
    uint32_t priorities;
    double utilization;
    char * end;
    std::string wfile;
    std::string tfile;

    while ((c = getopt_long(argc, argv, "w:u:c:t:p:", longOpts, NULL)) != -1)
	switch (c) {
	    case 'w':
		wfile = std::string(optarg);
		break;
	    case 'u':
		utilization = strtod(optarg, &end);
		break;
	    case 'p':
		priorities = strtol(optarg, &end, 10);
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
    std::cerr << "  - Priorities    : " << priorities << std::endl;
    std::cerr << "  - Completions   : " << comps << std::endl;
    std::cerr << "  - Trace File    : " << tfile << std::endl;


    std::vector<entry_t> * fifos = new std::vector<entry_t>[priorities];


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

    uint64_t size = 0;

    std::vector<entry_t> backing;

    // Simulation time
    uint64_t ts = 0;

    // If priorities = 24, the multi should be 1
    double bucket_size = ((double) priorities) / std::log2(10000000);

    std::cerr << bucket_size << std::endl;

    // Iterate through events until we reach the requisite number of completitions
    for (;simstat.comps < comps;) {

	uint64_t buff = 64;
	// If the small queue is not empty, decrement the head or pop it
	while (size != 0 && buff != 0) {
	    int fifo = 0;
	    for (; fifo < priorities; ++fifo) {
		if (!fifos[fifo].empty()) break;
	    }

	    entry_t head = fifos[fifo].front();
	    fifos[fifo].erase(fifos[fifo].begin());

	    if (head.ts != (*gqueue.begin()).ts && head.remaining != (*gqueue.begin()).remaining) {
		simstat.inacc++;
	    }

	    gqueue.erase(head);

	    head.remaining -= buff;

	    if (head.remaining > 0) {
		// log2(1MB) = 23.25 by default there are 24 bins
		// If you input you want 1000 priorities

		int bucket = std::log2(head.remaining) * bucket_size;
		fifos[bucket].insert(fifos[bucket].begin(), head);

		gqueue.insert(head);
		buff = 0;
	    } else {
			
		buff -= (buff + head.remaining);

		simstat.events++;
		simstat.sum_comps += (ts - head.ts);
		simstat.sum_slow_down += ((ts - head.ts) * 64)/head.length;
		simstat.comps++;
		size--;
	    }
	}

	// If the next arrival time is before or during this current timestep then add it to the queue
	while (std::floor(arrival/64.0) <= ts) {
	    entry_t ins = entry_t{ts, length, length};

	    assert(length != 0 && "initial length should not be 0");

	    int bucket = std::log2(length) * bucket_size;

	    size++;
	    fifos[bucket].push_back(ins);

	    gqueue.insert(ins);

	    simstat.events++;

	    // Move to the next arrival and length
	    arrival += arrivals(gen);
	    length  = generator(gen);
	}

	// Move the timestep to the next event
	ts += 1;
    }

    simstat.cycles = ts;

    std::cerr << "Total Completion : " << simstat.comps << std::endl;
    std::cerr << "Mean Completion  : " << ((double) simstat.sum_comps) / simstat.comps << std::endl;
    std::cerr << "Inaccuracies     : " << ((double) simstat.inacc) / simstat.cycles << std::endl;
    std::cerr << "Mean Slowdown    : " << ((double) simstat.sum_slow_down) / simstat.comps << std::endl;

    string slotstatroot = tfile;
    int fslotstat = open(slotstatroot.append(".slotstats").c_str(), O_RDWR | O_CREAT, 0644);
    write(fslotstat, &simstat, sizeof(simstat_t));
    close(fslotstat);
}
