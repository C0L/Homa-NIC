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
    uint32_t highwater;
    uint32_t lowwater;
    double compsum;
    double compcount;
};

struct slotstat_t {
    double validcycles;
    double totalmass;
    double statics;
};

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

    uint32_t * mlengths  = (uint32_t *) mmap(0, lsb.st_size, PROT_READ, MAP_PRIVATE, flength, 0);
    float * marrivals = (float *) mmap(0, asb.st_size, PROT_READ, MAP_PRIVATE, farrival, 0);

    // TODO max queue size
    slotstat_t * slotstats = (slotstat_t *) malloc(4*16384 * sizeof(slotstat_t));

    for (int i = 0; i < 4*16384; ++i) {
	slotstats[i].validcycles = 0;
	slotstats[i].totalmass = 0;
	slotstats[i].statics = 0;
    }

    uint64_t next = 0;
    uint64_t ts = 0;

    double arrival = marrivals[next];
    uint32_t length  = mlengths[next];

    next++;

    std::list<entry_t> hwqueue;
    std::list<entry_t> swqueue;
    std::queue<pair<uint32_t, entry_t>> chainup;
    std::queue<pair<uint32_t, entry_t>> chaindown;
    std::queue<entry_t> insert;

    uint32_t ring = 0;
    uint32_t swqueuetime = 0;

    uint32_t max = 0;

    // for (;ts < cycles; ++ts) {
    for (;simstat.compcount < cycles;) {
	bool push = false;
	bool pop = false;
	uint32_t insslot = 0;

	double tsf = (double) ts;

	uint32_t size = hwqueue.size();

	if (hwqueue.size() > max) {
	    max = hwqueue.size();
	    std::cerr << max << std::endl;
	}

	// Push to FIFO insert only if within cycle count
	if (arrival <= tsf) {
	    insert.push(entry_t{ts, length, length});
	    arrival += marrivals[next];
	    length  = (uint64_t) mlengths[next];

	    next++;
	}

	// If there are too many entries in queue, move them up
	if (hwqueue.size() >= highwater) {
	    simstat.highwater++;
	    uint32_t rem = blocksize;
	    while (!hwqueue.empty() && rem--) {
		// pair<uint64_t, uint32_t> tail = hwqueue.back();
		entry_t tail = hwqueue.back();

		// TODO need to tag with time here
		chainup.push(std::pair(ts, tail));
		// chainup.push(std::pair(ts, tail));
		hwqueue.pop_back();
	    }
	}

	// If there are not enough entries in queue, make a request for more
	if (hwqueue.size() <= lowwater && !swqueue.empty() && ring == 0) {
	    simstat.lowwater++;
	    // The on-chip queue can know the size of the offchip to determine whether to ring or not
	    ring = ts;
	}

	// Once the request for data has reached swqueue, begin satisfying it
	if (ts > ring + chainlatency && ring != 0) {
	    uint32_t rem = blocksize;
	    while (!swqueue.empty() && rem--) {
		chaindown.push(pair(ts, swqueue.front()));
		swqueue.pop_front();
	    }
	    ring = 0;
	}

	// if (swqueuetime != 0) {
	//     std::cerr << swqueuetime << std::endl;
	// }

	swqueuetime = (swqueuetime != 0) ? swqueuetime-1 : 0;

	if (!chainup.empty() && swqueuetime == 0) {
	    std::pair<uint32_t, entry_t> head = chainup.front();
	    if (head.first + chainlatency < ts) {
		chainup.pop();
		if (!srpt) {
		    // Insert FIFO
		    // TODO need to maintain FIFO order
		    // swqueue.push(ins);
		    // slot = hwqueue.size();
		} else {
		    // std::cerr << swqueue.size() << " " << hwqueue.size() << std::endl;
		    auto it = std::find_if(swqueue.begin(), swqueue.end(), [head](entry_t i) {return i.remaining > head.second.remaining;});
		    //slot = std::distance(swqueue.begin(), it);
		    swqueue.insert(it, head.second);
		}

		swqueuetime = sortdelay;
	    }
	}

	if (!chaindown.empty()) {
	    std::pair<uint32_t, entry_t> head = chaindown.front();
	    // Did it complete its sojurn time
	    if (head.first + chainlatency < ts) {
		chaindown.pop();
		insert.push(head.second);
	    }
	}

	// Pull from FIFO insert
	if (!insert.empty()) {
	    entry_t ins = insert.front();
	    insert.pop();
	    if (!srpt) {
		// Insert FIFO
		hwqueue.push_back(ins);
		insslot = hwqueue.size();
	    } else {
		auto it = std::find_if(hwqueue.begin(), hwqueue.end(), [ins](entry_t i) {return i.remaining > ins.remaining;});
		insslot = std::distance(hwqueue.begin(), it);
		hwqueue.insert(it, ins);
	    }
	    push = true;
	}

	// Pop from outgoing queue
	if (!hwqueue.empty()) {
	    entry_t head = hwqueue.front();
	    head.remaining--;
	    hwqueue.pop_front();
	    
	    if (head.remaining != 0) {
		hwqueue.push_front(head);
	    } else {
		pop = true;
		simstat.compsum += (ts - head.ts);
		// simstat.compsum += ((float) (ts - head.ts))/((float)(head.length));
		simstat.compcount++;
	    }
	}

	// Compute slot stats

	uint64_t mass = 0;
	auto it = hwqueue.begin();
	for (int i = 0; i < hwqueue.size(); ++i) {
	    slotstats[i].validcycles++;
	    slotstats[i].totalmass += mass;
	    mass += (*it).remaining ;
	    it++;
	}

	// Size is the original number of elements in the priority queue before enqueue/dequeue
	for (int s = 0; s < size; s++) {
	    int diff = 0;
	    if (pop) diff++;
	    if (s > insslot && push) diff--;
	    // if (diff == 0) {
	    slotstats[s].statics += diff;
	    // slotstats[s].statics += 1;
	    // }
	}

	// TODO wrap around
	if (ts > 1000000000) {
	    next = 0;
	}

	ts++;
    }

    std::cerr << "Total Cmpl: " << simstat.compcount << std::endl;
    std::cerr << "Mean Cmpl: " << simstat.compsum / simstat.compcount << std::endl;
    std::cerr << "Highwaters: " << simstat.highwater << std::endl;
    std::cerr << "Lowwaters: " << simstat.lowwater << std::endl;
    std::cerr << "Final Size: " << hwqueue.size() << std::endl;
    std::cerr << "Final Size: " << swqueue.size() << std::endl;
    // std::cerr << "Max: "       << max << std::endl;
    // std::cerr << "Max Mass: "  << maxmass << std::endl;

    std::ofstream rates;
    string ratesroot = tfile;
    rates.open(ratesroot.append(".rate").c_str());

    // TODO make this max queue size
    for (int i = 0; i < 4*16384; ++i) {
	if (slotstats[i].validcycles != 0) {
	    // rates << slotstats[i].statics / cycles << std::endl;
	    rates << slotstats[i].statics / slotstats[i].validcycles << std::endl;
	}
    }

    // TODO somehow incorporate the mass of the elements? (bytes/unit time). This would be more like mass flow?

    ofstream masses;
    string massroot = tfile;
    masses.open(massroot.append(".mass").c_str());

    // TODO make this max queue size
    for (int i = 0; i < 4*16384; ++i) {
	if (slotstats[i].validcycles != 0) {
	    // rates << slotstats[i].totalmass / cycles << std::endl;
	    masses << slotstats[i].totalmass / slotstats[i].validcycles << std::endl;
	}
    }

    std::ofstream simstats;
    string simstatroot = tfile;
    simstats.open(simstatroot.append(".simstats").c_str());

    simstats << simstat.compcount << std::endl;
    
    // TODO Normalize based on message length!! Then weight based on message length??
    simstats << simstat.compsum / simstat.compcount << std::endl;
    simstats << simstat.highwater << std::endl;
    simstats << simstat.lowwater << std::endl;
    simstats << hwqueue.size() << std::endl;
    simstats << swqueue.size() << std::endl;
    simstats << chaindown.size() << std::endl;
    simstats << chainup.size() << std::endl;

    rates.close();
    masses.close();
    simstats.close();

    close(flength);
    close(farrival);
}
