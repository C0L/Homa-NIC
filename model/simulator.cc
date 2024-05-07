#include <algorithm>
#include <iostream>
#include <list>
#include <fstream>

#include <cstdint>
#include <cmath>

static FILE * flengths;
static FILE * farrival;

struct log_t {
    float diffs;
    float count;
};

uint64_t ts = 0;

uint64_t next_length() {
 char line[256];
 char * end;
 fgets(line, sizeof(line), flengths);
 return strtol(line, &end, 10);
}

uint64_t next_arrival() {
 char line[256];
 char * end;
 fgets(line, sizeof(line), farrival);
 return strtol(line, &end, 10);
}

// Accept a workload file and interval file
int main(int argc, char ** argv) {

    std::string str("SRPT");

    bool srpt = str.compare(argv[1]) == 0;
    char * end;
    uint64_t cycles = strtol(argv[5], &end, 10);

    printf("cycles %d\n", cycles);
    printf("SRPT MODE %d\n", srpt);

    // Get flags from command line
    flengths = fopen(argv[2], "r");
    farrival = fopen(argv[3], "r");

    uint64_t arrival = next_arrival();
    uint64_t length  = next_length();

    while (length == 0) {
	arrival = next_arrival();
	length  = next_length();
    }

    std::list<uint64_t> queue;
    log_t stats[100000];

    for (int s = 0; s < 100000; ++s)  {
	stats[s].diffs = 0;
	stats[s].count = 0;
    }

    for (;ts < cycles; ++ts) {
	bool inserted = false;
	int slot = 0;

	// Enqueue 
	if (arrival <= ts) {
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
	
	    length = next_length();
	    arrival = next_arrival();

	    while (length == 0) {
		arrival = next_arrival();
		length  = next_length();
	    }
	} 

	bool pop = false;
	
	// Dequeue
	if (!queue.empty()) {
	    // std::cerr << queue.size() << std::endl;
	    uint64_t newhead = queue.front() - 1;
	    queue.pop_front();

	    if (newhead != 0) {
		queue.push_front(newhead);
	    } else {
		pop = true;
	    }
	}

	if (inserted && !pop) {
	    // everything up to slot+1 is 0, everything past is -1
	    int s = 0;
	    for (auto it = queue.begin(); it != std::prev(queue.end()); ++it) {
		if (s < slot) {
		    stats[s].count++;
		} else if (s >= slot) {
	     	    stats[s].diffs--;
		    stats[s].count++;
		}
		s++;
	    }
	} else if (inserted && pop) {
	    // everything up to slot+1 is 1, everything past is 0
	    int s = 0;
	    for (auto it = queue.begin(); it != queue.end(); ++it) {
		if (s < slot) {
	     	    stats[s].diffs++;
		    stats[s].count++;
		} else if (s >= slot) {
		    stats[s].count++;
		}
		s++;
	    }
	} else if (!inserted && pop){
	    // everything is 1
	    int s = 0;
	    for (auto it = queue.begin(); it != queue.end(); ++it) {
		stats[s].diffs++;
		stats[s].count++;
		s++;
	    }
	    stats[s].diffs++;
	    stats[s].count++;
	} else {
	    int s = 0;
	    for (auto it = queue.begin(); it != queue.end(); ++it) {
		stats[s].count++;
		s++;
	    }
	}
    }

    std::cerr << "COMPLETE SIM\n";

    // TODO get this name from cmd line
    std::ofstream myfile;
    myfile.open(argv[4]);
    // 40000
    for (int s = 0; s < 100000; ++s)  {
	if (stats[s].count != 0) {
	    float rate = stats[s].diffs/stats[s].count;
	    myfile << rate << std::endl;
	}
    }
    
    fclose(flengths);
    fclose(farrival);
}
