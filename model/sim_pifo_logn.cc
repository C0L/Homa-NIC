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

#include "dist.h"


using namespace std;

struct simstat_t {
    uint64_t highwater; // # of times highwater mark triggered
    uint64_t lowwater;  // # of times lowwater mark triggered
    uint64_t max;       // Maximum occupancy of the queue
    uint64_t events;    // # of events (push/pop)
    uint64_t compcount; // # of completions
    uint64_t compsum;   // Sum of MCTs
    uint64_t cycles;    // Total number of simulation cycles
};

struct entry_t {
    uint64_t ts;        // Start time
    uint32_t length;    // Original Length
    uint32_t remaining; // Remaining length
};

struct Compare {
    bool operator()(const entry_t & l, const entry_t & r) const {
        return l.remaining <= r.remaining;
    }
};

static simstat_t simstat; 

const struct option longOpts[] {
    {"workload",required_argument,NULL,'w'},
    {"comps",required_argument,NULL,'c'},
    {"utilization",required_argument,NULL,'u'},
    {"high-water",required_argument,NULL,'h'},
    {"low-water",required_argument,NULL,'l'},
    {"trace-file",required_argument,NULL,'t'},
    {"block-size",required_argument,NULL,'b'},
    {"sort-latency",required_argument,NULL,'s'}
};


int main(int argc, char ** argv) {
    int c;

    opterr = 0;

    uint64_t comps;
    double utilization;
    char * end;
    std::string wfile;
    std::string tfile;
    uint64_t highwater;
    uint64_t lowwater;
    uint32_t blocksize;
    uint32_t sortdelay;

    while ((c = getopt_long(argc, argv, "w:c:u:t:h:l:b:s:", longOpts, NULL)) != -1)
	switch (c) {
	    case 'w':
		wfile = std::string(optarg);
		break;
	    case 'c':
		comps = strtol(optarg, &end, 10);
		break;
	    case 'u':
		utilization = strtod(optarg, &end);
		break;
	    case 't':
		tfile = std::string(optarg);
		break;
	    case 'h':
		highwater = strtol(optarg, &end, 10);
		break;
	    case 'l':
		lowwater = strtol(optarg, &end, 10);
		break;
	    case 'b':
		blocksize = strtol(optarg, &end, 10);
		break;
	    case 's':
		sortdelay = strtol(optarg, &end, 10);
		break;
	    default:
		abort();
	}

    std::cerr << "Launching Simulator:" << std::endl;
    std::cerr << "  - Workload      : " << wfile << std::endl;
    std::cerr << "  - Utilizations  : " << utilization << std::endl;
    std::cerr << "  - Completions   : " << comps << std::endl;
    std::cerr << "  - Trace File    : " << tfile << std::endl;
    std::cerr << "  - High Water    : " << highwater << std::endl;
    std::cerr << "  - Low Water     : " << lowwater << std::endl;
    std::cerr << "  - Block Size    : " << blocksize << std::endl;
    std::cerr << "  - Sort Latency  : " << sortdelay << std::endl;


    std::random_device seed;
    std::mt19937 gen{seed()}; // seed the generator
    dist_point_gen generator(wfile.c_str(), 1000000);

    // for (size_t i = 0; i < 1000000; i++) {
    // 	generator(gen);
    // }

    // Want in units of sim cycles not bytes
    double desired_arrival = generator.get_mean() * (1.0/utilization) / 64.0;

    uint64_t ts = 0;

    double B = desired_arrival / std::tgamma((1+1/.1));

    std::weibull_distribution<> arrivals(.1, B);

    uint64_t arrival = std::round(arrivals(gen));
    uint32_t length = generator(gen) / 64.0;

    uint64_t next_event = 0;

    std::multiset<entry_t, Compare> hwqueue;
    std::multiset<entry_t, Compare> swqueue;
    std::queue<pair<uint64_t, entry_t>> pending;

    for (;simstat.compcount < comps;) {
    // for (;simstat.compcount < comps || !hwqueue.empty() || !swqueue.empty() || !pending.empty();) {

	// If the small queue is not empty, decrement the head or pop it
	if (!hwqueue.empty()) {
	    entry_t head = *hwqueue.begin(); 
	    hwqueue.erase(hwqueue.begin());
	    head.remaining -= next_event;

	    if (head.remaining != 0) {
		hwqueue.insert(head);
	    } else {
		simstat.events++;
		simstat.compsum += (ts - head.ts);
		simstat.compcount++;
	    }
	}

	// If the next arrival time is before or during this current timestep then add it to the queue
	while (arrival <= ts) {
	    entry_t ins = entry_t{ts, length, length};

	    if (length != 0) {
		hwqueue.insert(ins);
	    }

	    simstat.events++;

	    // Move to the next arrival and length
	    arrival += std::round(arrivals(gen));
	    length  = generator(gen) / 64.0;
	}

	// If there are too few entries in small queue, move them down
	if (hwqueue.size() <= lowwater && !swqueue.empty()) {
	    simstat.lowwater++;
	    uint32_t rem = blocksize;
	    while (!swqueue.empty() && rem--) {
		entry_t ins = *swqueue.begin();
		swqueue.erase(swqueue.begin());
		hwqueue.insert(ins);
	    }
	}

	// If there are too many entries in small queue, move them up
	if (hwqueue.size() >= highwater) {
	    simstat.highwater++;
	    uint32_t rem = blocksize;
	    while (!hwqueue.empty() && rem--) {
		// log2 of 0 not defined
		uint64_t ready = ts + sortdelay * ((uint64_t) log2(pending.size() + swqueue.size() + 1));

		entry_t tail = *std::prev(hwqueue.end());
		hwqueue.erase(std::prev(hwqueue.end()));
		pending.push(pair<uint64_t, entry_t>(ready, tail));
	    }
	}

	// TODO first == ts should be no different?
	while (!pending.empty() && pending.front().first <= ts) {
	    entry_t tail = pending.front().second;
	    pending.pop();
	    swqueue.insert(tail);
	}

	// entry_t head = *hwqueue.begin();
	// Determine the number of comps till the next event
	next_event = min({
		(!hwqueue.empty() ? (uint64_t) (*(hwqueue.begin())).remaining : (uint64_t) -1),
		(arrival - ts), 
		(!pending.empty() ? (uint64_t) (pending.front().first - ts) : (uint64_t) -1)
	    });

	// Move the timestep to the next event
	ts += next_event;
    }

    simstat.cycles = ts;

    std::cerr << "Total Cmpl: " << simstat.compcount << std::endl;
    std::cerr << "Comp Sum: " << simstat.compsum << std::endl;
    std::cerr << "Mean Cmpl: " << simstat.compsum / simstat.compcount << std::endl;
    std::cerr << "Highwaters: " << simstat.highwater << std::endl;
    std::cerr << "Lowwaters: " << simstat.lowwater << std::endl;
    std::cerr << "Cycles: " << simstat.cycles << std::endl;
    std::cerr << "Final Size: " << hwqueue.size() << std::endl;
    std::cerr << "Final Size: " << swqueue.size() << std::endl;

    string slotstatroot = tfile;
    int fslotstat = open(slotstatroot.append(".slotstats").c_str(), O_RDWR | O_CREAT, 0644);
    write(fslotstat, &simstat, sizeof(simstat_t));
    close(fslotstat);
}
