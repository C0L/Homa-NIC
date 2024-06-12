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
    uint64_t compsum;   // Some of MCTs
    uint64_t cycles;    // Total number of simulation cycles
    uint64_t qo;        // Sum queue Occupancy
};

struct slotstat_t {
    uint64_t validcycles;  // # of cycles a valid element is in this slot
    uint64_t totalbacklog; // Sum of mass of this slot and down
    uint64_t backlog;      // Sum of mass in this slot
    uint64_t mintotalbacklog = -1; // Minimum total backlog
    uint64_t minbacklog = -1;      // Minimum backlog
};

struct entry_t {
    uint64_t ts;        // Start time
    uint32_t length;    // Original Length
    uint32_t remaining; // Remaining length
};

struct Compare {
    bool operator()(const entry_t & l, const entry_t & r) const {
        return l.remaining < r.remaining;
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

    std::random_device seed;
    std::mt19937 gen{seed()}; // seed the generator
    dist_point_gen generator(wfile.c_str(), 1000000);

    for (size_t i = 0; i < 1000000; i++) {
	generator(gen);
    }

    // Want in units of sim cycles not bytes
    double desired_arrival = generator.get_mean() * (1.0/utilization) / 64.0;

    slotstat_t * slotstats;

    slotstats = (slotstat_t *) malloc(262144 * sizeof(slotstat_t));

    for (int i = 0; i < 262144; ++i) {
	slotstats[i].mintotalbacklog = -1;
	slotstats[i].minbacklog = -1;
    }

    uint64_t ts = 0;

    double B = desired_arrival / std::tgamma((1+1/.1));

    std::weibull_distribution<> arrivals(.1, B);

    uint64_t arrival = std::round(arrivals(gen));
    uint32_t length = generator(gen) / 64.0;

    uint64_t next_event = 0;

    std::multiset<entry_t, Compare> hwqueue;

    // Iterate through events until we reach the requisite number of completitions
    for (;simstat.compcount < comps;) {
	// std::cerr << arrival << std::endl;
	// std::cerr << length << std::endl;

	// Determine if we have reached a new maximum occupancy
	if (hwqueue.size() > simstat.max) {
	    simstat.max = hwqueue.size();
	}

	simstat.qo += next_event * hwqueue.size();

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

	next_event = min({
		(!hwqueue.empty() ? (uint64_t) (*(hwqueue.begin())).remaining : (uint64_t) -1),
		(uint64_t) (arrival - ts)
	    });

	    auto it = hwqueue.begin();

	    uint64_t mass = 0;
	    uint32_t i = 0;

	for(const auto & it : hwqueue) {
	    mass += it.remaining; 
	    slotstats[i].validcycles += next_event;
	    slotstats[i].totalbacklog += mass * next_event;
	    slotstats[i].backlog += it.remaining * next_event;
	    
	    if (slotstats[i].minbacklog > it.remaining) {
		slotstats[i].minbacklog = it.remaining;
	    }
	    
	    if (slotstats[i].mintotalbacklog > mass) {
		slotstats[i].mintotalbacklog = mass;
	    }
	    
	    i++;
	}



	// Move the timestep to the next event
	ts += next_event;
    }

    simstat.cycles = ts;

    std::cerr << "Total Cmpl: " << simstat.compcount << std::endl;
    std::cerr << "Mean Cmpl: " << simstat.compsum / simstat.compcount << std::endl;
    std::cerr << "Highwaters: " << simstat.highwater << std::endl;
    std::cerr << "Lowwaters: " << simstat.lowwater << std::endl;
    std::cerr << "Final Size: " << hwqueue.size() << std::endl;
    std::cerr << "Average Size: " << simstat.qo / simstat.cycles << std::endl;
    std::cerr << "Max Size: " << simstat.max << std::endl;

    string slotstatroot = tfile;
    int fslotstat = open(slotstatroot.append(".slotstats").c_str(), O_RDWR | O_CREAT, 0644);
    write(fslotstat, &simstat, sizeof(simstat_t));
    write(fslotstat, slotstats, 262144 * sizeof(slotstat_t));
    close(fslotstat);
}
