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
    float compsum;
    float compcount;
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
    {"block-size",required_argument,NULL,'b'}
};

// Maybe some ILP approach to derive the queue sizes that performs the best?

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
    uint32_t highwater;
    uint32_t lowwater;
    uint32_t chainlatency;
    uint32_t blocksize;

    while ((c = getopt_long(argc, argv, "q:d:a:c:t:h:l:p:b", longOpts, NULL)) != -1)
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

    int next = 0;
    uint64_t ts = 0;

    float arrival = marrivals[next];
    uint32_t length  = mlengths[next];

    next++;

    std::list<std::pair<uint64_t, uint32_t>> hwqueue;
    std::list<std::pair<uint64_t, uint32_t>> swqueue;
    std::queue<pair<uint32_t, std::pair<uint64_t, uint32_t>>> chainup;
    std::queue<pair<uint32_t, std::pair<uint64_t, uint32_t>>> chaindown;
    std::queue<std::pair<uint64_t, uint32_t>> insert;

    uint32_t ring = 0;
    for (;ts < cycles; ++ts) {

	// std::cerr << arrival << std::endl;
	// std::cerr << length << std::endl;

	float tsf = (float) ts;
	// Push to FIFO insert
	if (arrival <= tsf) {
	    insert.push(std::pair(length, ts));
	    arrival += marrivals[next];
	    length  = (uint64_t) mlengths[next];
	    next++;
	} 

	// If there are too many entries in queue, move them up
	if (hwqueue.size() > highwater) {
	    simstat.highwater++;
	    uint32_t rem = blocksize;
	    while (!hwqueue.empty() && rem--) {
		pair<uint64_t, uint32_t> tail = hwqueue.back();

		// TODO need to tag with time here
		chainup.push(std::pair(ts, tail));
		hwqueue.pop_back();
	    }
	}

	// If there are not enough entries in queue, make a request for more
	if (hwqueue.size() < lowwater && !swqueue.empty() && ring == 0) {
	    simstat.lowwater++;
	    // TODO this will just spam requests
	    // The on-chip queue can know the size of the offchip to determine whether to ring or not
	    ring = ts;
	    // std::cerr << "ring!!!" << std::endl;
	}
	// else if (hwqueue.size() < lowwater && !swqueue.empty()) {
	//     std::cerr << "BING" << std::endl;
	// }

	// if (hwqueue.size() < lowwater) {
	//     std::cerr << "ring!!!" << std::endl;
	// }

	// Once the request for data has reached swqueue, begin satisfying it
	if (ts > ring + chainlatency && ring != 0) {
	    uint32_t rem = blocksize;
	    while (!swqueue.empty() && rem--) {
		// std::cerr << "CHAIN DOWN\n";
		chaindown.push(pair(ts, swqueue.front()));
		swqueue.pop_front();
	    }
	    ring = 0;
	}

	if (!chainup.empty()) {
	    std::pair<uint32_t, std::pair<uint64_t, uint32_t>> head = chainup.front();
	    if (head.first + chainlatency < ts) {
		chainup.pop();
		// TODO sort slowly here
		if (!srpt) {
		    std::cerr << "Unimplemented" << std::endl;
		    // Insert FIFO
		    // TODO need to maintain FIFO order
		    // swqueue.push(ins);
		    // slot = hwqueue.size();
		} else {
		    // std::cerr << swqueue.size() << " " << hwqueue.size() << std::endl;
		    auto it = std::find_if(swqueue.begin(), swqueue.end(), [head](std::pair<uint64_t, uint32_t> i) {return i.first > head.second.first;});
		    //slot = std::distance(swqueue.begin(), it);
		    swqueue.insert(it, head.second);
		}
	    }
	}

	if (!chaindown.empty()) {
	    std::pair<uint32_t, std::pair<uint64_t, uint32_t>> head = chaindown.front();
	    // Did it complete its sojurn time
	    if (head.first + chainlatency < ts) {
		chaindown.pop();
		insert.push(head.second);
	    }
	}

	// Pull from FIFO insert
	if (!insert.empty()) {
	    std::pair<uint64_t, uint32_t> ins = insert.front();
	    insert.pop();
	    if (!srpt) {
		// Insert FIFO
		hwqueue.push_back(ins);
		// slot = hwqueue.size();
	    } else {
		auto it = std::find_if(hwqueue.begin(), hwqueue.end(), [ins](std::pair<uint64_t, uint32_t> i) {return i.first > ins.first;});
		// slot = std::distance(hwqueue.begin(), it);
		hwqueue.insert(it, ins);
	    }
	}

	// Pop from outgoing queue
	if (!hwqueue.empty()) {
	    std::pair<uint64_t, uint32_t> head = hwqueue.front();
	    head.first--;
	    hwqueue.pop_front();
	    
	    if (head.first != 0) {
		hwqueue.push_front(head);
	    } else {
		simstat.compsum += (ts - head.second);
		simstat.compcount++;
	    }
	}

	// Size is the original number of elements in the priority queue before enqueue/dequeue
	// for (int s = 0; s < size; s++) {
	//     int diff = 0;
	//     if (pop) diff++;
	//     if (s >= slot && inserted) diff--;
	//     stats[s].diffs += diff;
	//     stats[s].count++;
	// }
    }

    // std::ofstream rates;
    // rates.open(tfile.c_str());
    // float compsum = 0;
    // float compcount = 0;
    // for (int s = 0; s < cycles; ++s)  {
    // 	if (stats[s].start != 0) {
    // 	    compsum += (stats[s].end - stats[s].start);
    // 	    compcount++;
    // 	}
    // }
    // rates << compsum/compcount << std::endl;
    // rates.close();

    std::cerr << "Total Cmpl: " << simstat.compcount << std::endl;
    std::cerr << "Mean Cmpl: " << simstat.compsum / simstat.compcount << std::endl;
    std::cerr << "Highwaters: " << simstat.highwater << std::endl;
    std::cerr << "Lowwaters: " << simstat.lowwater << std::endl;
    std::cerr << "Final Size: " << hwqueue.size() << std::endl;
    std::cerr << "Final Size: " << swqueue.size() << std::endl;
    // std::ofstream rates;
    // rates.open(tfile.c_str());
    // for (int s = 0; s < cycles; ++s)  {
    // 	if (stats[s].count != 0) {
    // 	    float rate = stats[s].diffs/stats[s].count;
    // 	    rates << rate << std::endl;
    // 	}
    // }
    // rates.close();

    close(flength);
    close(farrival);

}
