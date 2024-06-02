#include <algorithm>
#include <iostream>
#include <list>
#include <queue>
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

using namespace std;

struct simstat_t {
    uint64_t highwater; // # of times highwater mark triggered
    uint64_t lowwater;  // # of times lowwater mark triggered
    uint64_t max;       // Maximum occupancy of the queue
    uint64_t events;    // # of events (push/pop)
    uint64_t compcount; // # of completions
    uint64_t compsum;   // Some of MCTs
    uint64_t cycles;    // Total number of simulation cycles
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

static simstat_t simstat; 
static slotstat_t slotstats[16384]; // TODO assumes max queue size

const struct option longOpts[] {
    {"queue-type",required_argument,NULL,'q'},
    {"length-file",required_argument,NULL,'d'},
    {"arrival-file",required_argument,NULL,'a'},
    {"comps",required_argument,NULL,'c'},
    // {"warmup",required_argument,NULL,'w'},
    // {"drain",no_argument,NULL,'r'},
    {"trace-file",required_argument,NULL,'t'},
    {"high-water",required_argument,NULL,'h'},
    {"low-water",required_argument,NULL,'l'},
    {"chain-latency",required_argument,NULL,'p'},
    {"block-size",required_argument,NULL,'b'},
    {"sort-latency",required_argument,NULL,'s'}
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
    float sortdelay;
    // uint32_t sortdelay;
    // uint64_t warmup;
    // bool drain = false;

    while ((c = getopt_long(argc, argv, "q:d:a:c:t:h:l:p:b:s", longOpts, NULL)) != -1)
	switch (c) {
	    case 'q':
		type = std::string(optarg);
		break;
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
	    case 'h':
		highwater = strtol(optarg, &end, 10);
		break;
	    case 'l':
		lowwater = strtol(optarg, &end, 10);
		break;
	    case 'p':
		chainlatency = strtol(optarg, &end, 10);
		break;
	    case 'b':
		blocksize = strtol(optarg, &end, 10);
		break;
	    case 's':
		sortdelay = strtof(optarg, &end);
		// sortdelay = strtol(optarg, &end, 10);
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
    std::cerr << "  - High Water    : " << highwater << std::endl;
    std::cerr << "  - Low Water     : " << lowwater << std::endl;
    std::cerr << "  - Chain Latency : " << chainlatency << std::endl;
    std::cerr << "  - Block Size    : " << blocksize << std::endl;
    std::cerr << "  - Sort Latnecy  : " << sortdelay << std::endl;

    // TODO check everything was set

    std::string str("SRPT");

    bool srpt = str.compare(type) == 0;

    struct stat lsb;
    struct stat asb;

    int flength  = open(lfile.c_str(), O_RDONLY);
    int farrival = open(afile.c_str(), O_RDONLY);

    fstat(flength, &lsb);
    fstat(farrival, &asb);

    uint32_t * mlengths  = (uint32_t *) mmap(0, lsb.st_size, PROT_READ, MAP_PRIVATE, flength, 0);
    float * marrivals    = (float *) mmap(0, asb.st_size, PROT_READ, MAP_PRIVATE, farrival, 0);

    // if (comps >= lsb.st_size/4 || comps >= asb.st_size/4) {
    // 	perror("insufficient trace data");
    // }

    uint64_t next = 0;
    uint64_t ts = 0;

    double arrival  = marrivals[next];
    uint32_t length = mlengths[next];

    next++;

    std::list<entry_t> hwqueue;
    std::list<entry_t> swqueue;
    // std::queue<entry_t> swsort;

    uint64_t next_event = 0;

    uint64_t sort = 0;

    // Iterate through events until we reach the requisite number of completitions
    for (;simstat.compcount < comps;) {

	// Determine if we have reached a new maximum occupancy
	if (hwqueue.size() + swqueue.size() > simstat.max) {
	    simstat.max = hwqueue.size() + swqueue.size();
	}

	/*
	auto it = hwqueue.begin();

	uint64_t mass = 0;
	for (int i = 0; i < hwqueue.size(); ++i) {
	    if (i == 0) {
		// TODO check this logic
		if (next_event % 2 == 1) {
		    mass = ((*it).remaining - (next_event/2)) * next_event - next_event;
		} else {
		    mass = (((*it).remaining - (next_event/2))) * next_event;
		}

		slotstats[0].validcycles += next_event;
		slotstats[0].totalbacklog += mass;
		slotstats[0].backlog += mass;
	    } else {
		mass += (*it).remaining * next_event;
		slotstats[i].validcycles += next_event;
		slotstats[i].totalbacklog += mass;
		slotstats[i].backlog += (*it).remaining * next_event;
	    }


	    if (slotstats[i].minbacklog > (*it).remaining) {
		slotstats[i].minbacklog = (*it).remaining;
	    }

	    if (slotstats[i].mintotalbacklog > mass) {
		slotstats[i].mintotalbacklog = mass;
	    }

	    it++;
	}
	*/
	
	// If the small queue is not empty, decrement the head or pop it
	if (!hwqueue.empty()) {
	    entry_t head = hwqueue.front();
	    head.remaining -= next_event;
	    hwqueue.pop_front();
	    
	    if (head.remaining != 0) {
		hwqueue.push_front(head);
	    } else {
		simstat.events++;
		simstat.compsum += (ts - head.ts);
		simstat.compcount++;
	    }
	}

	// If the next arrival time is before or during this current timestep then add it to the queue
	while (arrival <= ts) {
	    entry_t ins = entry_t{ts, length, length};

	    auto it = std::find_if(hwqueue.begin(), hwqueue.end(), [ins](entry_t i) {return i.remaining > ins.remaining;});
	    hwqueue.insert(it, ins);
	    simstat.events++;


	    // Move to the next arrival and length
	    arrival += marrivals[next];
	    length  =  mlengths[next];

	    next++;
	}

	// If there are too few entries in small queue, move them down
	if (hwqueue.size() <= lowwater && !swqueue.empty()) {
	    simstat.lowwater++;
	    uint32_t rem = blocksize;
	    while (!swqueue.empty() && rem--) {
		entry_t ins = swqueue.front();

		auto it = std::find_if(hwqueue.begin(), hwqueue.end(), [ins](entry_t i) {return i.remaining > ins.remaining;});
		hwqueue.insert(it, ins);
		swqueue.pop_front();
	    }
	}

	// If there are too many entries in small queue, move them up
	if (hwqueue.size() >= highwater) {
	    simstat.highwater++;
	    uint32_t rem = blocksize;
	    while (!hwqueue.empty() && rem--) {
		entry_t tail = hwqueue.back();
		// Vary the accuracy here?
		// swqueue.push_front(tail);

		auto it = std::find_if(swqueue.begin(), swqueue.end(), [tail](entry_t i) {return i.remaining > tail.remaining;});

		double r = ((double) rand() / (RAND_MAX));

		if (r < sortdelay) {
		    // Random insertion
		    double ins = ((double) rand() / (RAND_MAX)) * swqueue.size();
		    it = std::next(swqueue.begin(), (int) ins);
		} 

		swqueue.insert(it, tail);

		hwqueue.pop_back();
	    }

	    // uint32_t rem = blocksize;
	    // while (!hwqueue.empty() && rem--) {
	    // 	entry_t tail = hwqueue.back();
	    // 	auto it = std::find_if(swqueue.begin(), swqueue.end(), [tail](entry_t i) {return i.remaining > tail.remaining;});
	    // 	swqueue.insert(it, tail);

	    // 	hwqueue.pop_back();
	    // }
	}

	// if (sort % sortdelay == 0 && !swsort.empty()) {
	//     entry_t tail = swsort.front();
	//     auto it = std::find_if(swqueue.begin(), swqueue.end(), [tail](entry_t i) {return i.remaining > tail.remaining;});
	//     swqueue.insert(it, tail);
	//     swsort.pop();
	//     sort = 0;
	// }

	// Determine the number of comps till the next event
	next_event = min((uint64_t) ((!hwqueue.empty()) ? hwqueue.front().remaining : -1), ((uint64_t) arrival + 1) - ts);

	// Move the timestep to the next event
	ts += next_event;
    }

    simstat.cycles = ts;

    std::cerr << "Total Cmpl: " << simstat.compcount << std::endl;
    std::cerr << "Mean Cmpl: " << simstat.compsum / simstat.compcount << std::endl;
    std::cerr << "Highwaters: " << simstat.highwater << std::endl;
    std::cerr << "Lowwaters: " << simstat.lowwater << std::endl;
    std::cerr << "Final Size: " << hwqueue.size() << std::endl;
    std::cerr << "Final Size: " << swqueue.size() << std::endl;

    string slotstatroot = tfile;
    int fslotstat = open(slotstatroot.append(".slotstats").c_str(), O_RDWR | O_CREAT, 0644);
    write(fslotstat, &simstat, sizeof(simstat_t));
    // write(fslotstat, &slotstats, 16384 * sizeof(slotstat_t));
    close(fslotstat);

    close(flength);
    close(farrival);
}
