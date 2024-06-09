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
        return l.remaining <= r.remaining;
    }
};

static simstat_t simstat; 

const struct option longOpts[] {
    {"length-file",required_argument,NULL,'d'},
    {"arrival-file",required_argument,NULL,'a'},
    {"comps",required_argument,NULL,'c'},
    {"trace-file",required_argument,NULL,'t'},
};

int main(int argc, char ** argv) {
    int c;

    opterr = 0;

    uint64_t comps;
    char * end;
    std::string type;
    std::string lfile;
    std::string afile;
    std::string tfile;
    uint64_t highwater;
    uint64_t lowwater;
    uint32_t chainlatency;
    uint32_t blocksize;
    double sortdelay;

    while ((c = getopt_long(argc, argv, "d:a:c:t:", longOpts, NULL)) != -1)
	switch (c) {
	    case 'd':
		lfile = std::string(optarg);
		break;
	    case 'a':
		afile = std::string(optarg);
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
    std::cerr << "  - Queuing Policy: " << type << std::endl;
    std::cerr << "  - Length File   : " << lfile << std::endl;
    std::cerr << "  - Arrival File  : " << afile << std::endl;
    std::cerr << "  - Completions   : " << comps << std::endl;
    std::cerr << "  - Trace File    : " << tfile << std::endl;

    struct stat lsb;
    struct stat asb;

    int flength  = open(lfile.c_str(), O_RDONLY);
    int farrival = open(afile.c_str(), O_RDONLY);

    fstat(flength, &lsb);
    fstat(farrival, &asb);

    if (comps >= lsb.st_size/4 || comps >= asb.st_size/4) {
	perror("insufficient trace data");
	exit(1);
    }

    uint32_t * mlengths  = (uint32_t *) mmap(0, lsb.st_size, PROT_READ, MAP_PRIVATE, flength, 0);
    float * marrivals    = (float *) mmap(0, asb.st_size, PROT_READ, MAP_PRIVATE, farrival, 0);
    slotstat_t * slotstats;

    slotstats = (slotstat_t *) malloc(262144 * sizeof(slotstat_t));

    for (int i = 0; i < 262144; ++i) {
	slotstats[i].mintotalbacklog = -1;
	slotstats[i].minbacklog = -1;
    }

    uint64_t next = 0;
    uint64_t ts = 0;

    double arrival  = marrivals[next];
    uint32_t length = mlengths[next];

    next++;

    std::multiset<entry_t, Compare> hwqueue;
    std::queue<pair<uint64_t, entry_t>> pending;

    uint64_t next_event = 0;
    uint64_t last_samp = 0;

    // Iterate through events until we reach the requisite number of completitions
    // for (;simstat.compcount < comps;) {
    for (;simstat.compcount < comps || !hwqueue.empty();) {

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

		if (simstat.compcount == comps)  {
		    arrival = std::numeric_limits<double>::max();
		}
	    }
	}

	// If the next arrival time is before or during this current timestep then add it to the queue
	while (arrival <= ts) {
	    entry_t ins = entry_t{ts, length, length};

	    hwqueue.insert(ins);

	    simstat.events++;

	    // Move to the next arrival and length
	    arrival += marrivals[next];
	    length  =  mlengths[next];

	    next++;
	}

	next_event = min(
		(!hwqueue.empty() ? (uint64_t) (*(hwqueue.begin())).remaining : (uint64_t) -1),
		(simstat.compcount < comps ? (uint64_t) (arrival + 1 - ts) : (uint64_t) -1)
	    );

	if (next_event == -1) {
	    next_event = 1;
	}

	// if (ts % 100000 == 0) {
	//     std::cerr << ts << std::endl;
	// }

	// Move the timestep to the next event
	ts += next_event;


	if (last_samp > 10000) {
	    auto it = hwqueue.begin();

	    uint64_t mass = 0;
	    uint32_t i = 0;
	    for(const auto & it : hwqueue) {
	    	mass += it.length; // TODO remaining?
	    	slotstats[i].validcycles += 1;
	    	slotstats[i].totalbacklog += mass;
	    	slotstats[i].backlog += it.length;

	    	if (slotstats[i].minbacklog > it.length) {
	    	    slotstats[i].minbacklog = it.length;
	    	}

	    	if (slotstats[i].mintotalbacklog > mass) {
	    	    slotstats[i].mintotalbacklog = mass;
	    	}

		i++;
	    }

	    // auto it = hwqueue.begin();

	    // uint64_t mass = 0;
	    // for (int i = 0; i < hwqueue.size(); ++i) {
	    // 	mass += (*it).remaining;
	    // 	slotstats[i].validcycles += 1;
	    // 	slotstats[i].totalbacklog += mass;
	    // 	// if (slotstats[i].totalbacklog > ((uint64_t) -1)/2) {
	    // 	//     std::cerr << "BAD SIGN" << std::endl;
	    // 	//     exit(1);
	    // 	// }
	    // 	slotstats[i].backlog += (*it).remaining;
	    // 	// slotstats[i].backlog += (*it).remaining * next_event;

	    // 	if (slotstats[i].minbacklog > (*it).remaining) {
	    // 	    slotstats[i].minbacklog = (*it).remaining;
	    // 	}

	    // 	if (slotstats[i].mintotalbacklog > mass) {
	    // 	    slotstats[i].mintotalbacklog = mass;
	    // 	}

	    // 	it++;
	    // }
	    last_samp = last_samp - 10000 + next_event;
	} else {
	    last_samp += next_event;
	}
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

    close(flength);
    close(farrival);
}
