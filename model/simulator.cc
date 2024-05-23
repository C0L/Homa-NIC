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
    uint64_t highwater;
    uint64_t lowwater;
    uint64_t max;
    uint64_t events;
    uint64_t compcount;
    double   compsum;
};

// struct slotstat_t {
//     double validcycles;
//     double totalmass;
//     double statics;
// };

struct entry_t {
    uint64_t ts;        // Start time
    uint32_t length;    // Original Length
    uint32_t remaining; // Remaining length
};

static simstat_t simstat;

const struct option longOpts[]{
    {"queue-type",required_argument,NULL,'q'},
    {"length-file",required_argument,NULL,'d'},
    {"arrival-file",required_argument,NULL,'a'},
    {"cycles",required_argument,NULL,'c'},
    {"trace-file",required_argument,NULL,'t'},
    {"high-water",required_argument,NULL,'h'},
    {"low-water",required_argument,NULL,'l'},
    {"chain-latency",required_argument,NULL,'p'},
    {"block-size",required_argument,NULL,'b'},
    {"sort-latency",required_argument,NULL,'s'}
};

// Maybe some ILP approach to derive the queue sizes that performs the best?

// Assume that data is X many cycles away over pcie
// Configuration 1:
//   - Data coming out of the queue gets delayed for PCie latency
//   - End time becomes current + pcie latency
// Configuration 2:
//   - We are prefetching data at a rate of 1.5x, based on SRPT order
//   - TODO is this really meaningful without grant system/flow control?
//   - Maybe emulate these with some poisson generator???

/*
 * highwater - trggers data moving off chip
 * lowwater  - triggers data moving on chip
 */
int main(int argc, char ** argv) {
    int c;

    opterr = 0;

    uint64_t cycles;
    char * end;
    std::string type;
    std::string lfile;
    std::string afile;
    std::string tfile;
    uint64_t highwater;
    uint64_t lowwater;
    uint32_t chainlatency;
    uint32_t blocksize;
    uint32_t sortdelay;

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
		cycles = strtol(optarg, &end, 10);
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
		sortdelay = strtol(optarg, &end, 10);
		break;
	    default:
		abort ();
	}

    std::cerr << "Launching Simulator:" << std::endl;
    std::cerr << "  - Queuing Policy: " << type << std::endl;
    std::cerr << "  - Length File   : " << lfile << std::endl;
    std::cerr << "  - Arrival File  : " << afile << std::endl;
    std::cerr << "  - Cycles        : " << cycles << std::endl;
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

    uint32_t * mlengths  = (uint32_t *) mmap(0, lsb.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, flength, 0);
    float * marrivals    = (float *) mmap(0, asb.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, farrival, 0);

    uint64_t next = 0;
    uint64_t ts = 0;

    double arrival = marrivals[next];
    uint32_t length  = mlengths[next];

    next++;

    std::list<entry_t> hwqueue;
    std::list<entry_t> swqueue;
    // std::queue<pair<uint32_t, entry_t>> chainup;
    // std::queue<pair<uint32_t, entry_t>> chaindown;
    // std::queue<entry_t> insert;

    simstat.highwater = 0;
    simstat.lowwater  = 0;
    simstat.max       = 0;
    simstat.events    = 0;
    simstat.compcount = 0;
    simstat.compsum   = 0;

    uint64_t next_event = 0;

    // Iterate through events until we reach the requisite number of completitions
    for (;simstat.compcount < cycles;) {

	// bool event = false;

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

	while (arrival <= ts) {
	    entry_t ins = entry_t{ts, length, length};
	    arrival += marrivals[next];
	    length  =  mlengths[next];

	    auto it = std::find_if(hwqueue.begin(), hwqueue.end(), [ins](entry_t i) {return i.remaining > ins.remaining;});
	    hwqueue.insert(it, ins);
	    simstat.events++;

	    next++;
	}

	// If there are too many entries in queue, move them up
	if (hwqueue.size() >= highwater) {
	    simstat.highwater++;
	    uint32_t rem = blocksize;
	    while (!hwqueue.empty() && rem--) {
		entry_t tail = hwqueue.back();
		auto it = std::find_if(swqueue.begin(), swqueue.end(), [tail](entry_t i) {return i.remaining > tail.remaining;});
		swqueue.insert(it, tail);

		hwqueue.pop_back();
	    }
	}

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

	if (hwqueue.size() + swqueue.size() > simstat.max) {
	    simstat.max = hwqueue.size() + swqueue.size();
	}

	// TODO wrap around
	if (next >= 10000000) {
	    std::cerr << "Wrap" << std::endl;
	    next = 0;
	}

	// std::cerr << arrival << std::endl;
	// std::cerr << hwqueue.front().remaining << std::endl;
	next_event = min((uint64_t) ((!hwqueue.empty()) ? hwqueue.front().remaining : -1), ((uint64_t) arrival) - ts);

	// std::cerr << next_event << std::endl;
    
	ts += next_event;
    }

    std::cerr << "Total Cmpl: " << simstat.compcount << std::endl;
    std::cerr << "Mean Cmpl: " << simstat.compsum / simstat.compcount << std::endl;
    std::cerr << "Highwaters: " << simstat.highwater << std::endl;
    std::cerr << "Lowwaters: " << simstat.lowwater << std::endl;
    std::cerr << "Final Size: " << hwqueue.size() << std::endl;
    std::cerr << "Final Size: " << swqueue.size() << std::endl;

    string simstatroot = tfile;
    int statfile = open(simstatroot.append(".simstats").c_str(), O_RDWR | O_CREAT, 0644);
    write(statfile, &simstat, sizeof(simstat_t));
    close(statfile);

    close(flength);
    close(farrival);
}
