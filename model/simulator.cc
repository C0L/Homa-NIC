#include <algorithm>
#include <iostream>
#include <list>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <cstdint>
#include <cmath>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int flength;
static int farrival;

struct log_t {
    float diffs;
    float count;
};


// TODO clean this up then start working on high/low water mark implementation

// Accept a workload file and interval file
int main(int argc, char ** argv) {
    // char * qtype = NULL;
    // int index;

    int c;

    opterr = 0;

    uint64_t cycles;
    char * end;
    std::string type;
    std::string lfile;
    std::string afile;
    std::string tfile;

    while ((c = getopt(argc, argv, "q:l:a:c:t:")) != -1)
	switch (c) {
	    case 'q':
		type = std::string(optarg);
		break;
	    case 'l':
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
	    default:
		abort ();
	}

    std::cerr << "Launching Simulator:" << std::endl;
    std::cerr << "  - Queuing Policy: " << type << std::endl;
    std::cerr << "  - Lengths File  : " << lfile << std::endl;
    std::cerr << "  - Arrival File  : " << afile << std::endl;
    std::cerr << "  - Trace File    : " << tfile << std::endl;
    std::cerr << "  - Cycles        : " << cycles << std::endl;

    // TODO check everything was set

    std::string str("SRPT");

    bool srpt = str.compare(type) == 0;

    struct stat lsb;
    struct stat asb;

    flength  = open(lfile.c_str(), O_RDONLY);
    farrival = open(afile.c_str(), O_RDONLY);

    fstat(flength, &lsb);
    fstat(farrival, &asb);

    uint64_t * mlengths  = (uint64_t *) mmap(0, lsb.st_size, PROT_READ, MAP_PRIVATE, flength, 0);
    uint64_t * marrivals = (uint64_t *) mmap(0, asb.st_size, PROT_READ, MAP_PRIVATE, farrival, 0);

    int next = 0;
    uint64_t ts = 0;

    uint64_t arrival = marrivals[next];
    uint64_t length  = mlengths[next];

    next++;

    std::list<uint64_t> queue;
    std::queue<std::pair<uint64_t, uint64_t>> insert;

    log_t * stats = (log_t *) malloc(cycles * sizeof(log_t));

    for (int s = 0; s < cycles; ++s)  {
	stats[s].diffs = 0;
	stats[s].count = 0;
    }

    for (;ts < cycles; ++ts) {
	bool inserted = false; // Did we insert this cycle
	bool pop = false;     // Did we pop this cycle
	int slot = 0;
	int size = queue.size();

	if (arrival <= ts) {
	    insert.push_back();

	    arrival = marrivals[next];
	    length  = mlengths[next];
	    next++;
	} 

	// Enqueue
	if (!insert.empty()) {
	    inserted = true;
	    if (!srpt) {
		// Insert FIFO
		queue.push_back(length);
		slot = queue.size();
	    } else {
		auto it = std::find_if(queue.begin(), queue.end(), [length](uint64_t i) {return i > length;});
		    slot = std::distance(queue.begin(), it);
		    queue.insert(it, length);
	    }
	}

	// Dequeue
	if (!queue.empty()) {
	    uint64_t newhead = queue.front() - 1;
	    queue.pop_front();
	    
	    if (newhead != 0) {
		queue.push_front(newhead);
	    } else {
		pop = true;
	    }
	}
	
	// Size is the original number of elements in the priority queue before enqueue/dequeue
	for (int s = 0; s < size; s++) {
	    int diff = 0;
	    if (pop) diff++;
	    if (s >= slot && inserted) diff--;
	    stats[s].diffs += diff;
	    stats[s].count++;
	}
    }

    std::ofstream rates;
    rates.open(tfile.c_str());
    // 40000
    for (int s = 0; s < cycles; ++s)  {
	if (stats[s].count != 0) {
	    float rate = stats[s].diffs/stats[s].count;
	    rates << rate << std::endl;
	}
    }


    close(flength);
    close(farrival);
    rates.close();
    // fclose(farrival);
}
