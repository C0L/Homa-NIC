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
};

struct entry_t {
    uint64_t ts;        // Start time
    uint32_t length;    // Original Length
    uint32_t remaining; // Remaining length
};

static simstat_t simstat; 

const struct option longOpts[] {
    {"queue-type",required_argument,NULL,'q'},
    {"length-file",required_argument,NULL,'d'},
    {"arrival-file",required_argument,NULL,'a'},
    {"comps",required_argument,NULL,'c'},
    {"trace-file",required_argument,NULL,'t'},
    {"high-water",required_argument,NULL,'h'},
    {"low-water",required_argument,NULL,'l'},
    {"chain-latency",required_argument,NULL,'p'},
    {"block-size",required_argument,NULL,'b'},
    {"sort-latency",required_argument,NULL,'s'},
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
    uint32_t sortdelay;

    while ((c = getopt_long(argc, argv, "q:d:a:c:t:h:l:p:b:s:", longOpts, NULL)) != -1)
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
		sortdelay = strtol(optarg, &end, 10);
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
    std::cerr << "  - Sort Latenecy  : " << sortdelay << std::endl;

    struct stat lsb;
    struct stat asb;

    int flength  = open(lfile.c_str(), O_RDONLY);
    int farrival = open(afile.c_str(), O_RDONLY);

    fstat(flength, &lsb);
    fstat(farrival, &asb);

    uint32_t * mlengths  = (uint32_t *) mmap(0, lsb.st_size, PROT_READ, MAP_PRIVATE, flength, 0);
    float * marrivals    = (float *) mmap(0, asb.st_size, PROT_READ, MAP_PRIVATE, farrival, 0);

    if (comps >= lsb.st_size/4 || comps >= asb.st_size/4) {
	perror("insufficient trace data");
    }

    uint64_t next = 0;
    uint64_t ts = 0;

    double arrival  = marrivals[next];
    uint32_t length = mlengths[next];

    next++;

    std::list<entry_t> hwqueue;
    std::list<entry_t> swqueue;
    std::queue<pair<uint64_t, entry_t>> pending;

    uint64_t next_event = 0;
    uint64_t sort = 0;

    // Iterate through events until we reach the requisite number of completitions
    for (;simstat.compcount < comps || !hwqueue.empty() || !swqueue.empty() || !pending.empty();) {

	// Determine if we have reached a new maximum occupancy
	if (hwqueue.size() + swqueue.size() > simstat.max) {
	    simstat.max = hwqueue.size() + swqueue.size();
	}

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

		if (simstat.compcount == comps)  {
		    std::cerr << "COMPLETE\n";
		    arrival = std::numeric_limits<double>::max();    // TODO THIS does not disable arrival!!!!!
		}
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
		uint64_t ready = ts + sortdelay * ((uint64_t) log2(pending.size() + swqueue.size()));

		if (ready != ts) std::cerr << "FAILURE" << std::endl;

		// std::cerr << "check" << std::endl;

	
		entry_t tail = hwqueue.back();
		hwqueue.pop_back();
		pending.push(pair<uint64_t, entry_t>(ready, tail));
		bool test =  !pending.empty() && pending.front().first <= ts;

		// std::cerr << test << std::endl;

	    }
	}

	// TODO first == ts should be no different?
	while (!pending.empty() && pending.front().first <= ts) {
	    std::cerr << "NOW ACTIVE \n\n\n\n\n\n\n\n\n" << std::endl;
		entry_t tail = pending.front().second;
		pending.pop();
		auto it = std::find_if(swqueue.begin(), swqueue.end(), [tail](entry_t i) {return i.remaining > tail.remaining;});
		swqueue.insert(it, tail);
	}

	// Determine the number of comps till the next event
	next_event = min({
		(uint64_t) (!hwqueue.empty() ? hwqueue.front().remaining : -1),
		(uint64_t) (arrival + 1 - ts),
		(uint64_t) (!pending.empty() ? pending.front().first - ts : -1)
	    });

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
    close(fslotstat);

    close(flength);
    close(farrival);
}
