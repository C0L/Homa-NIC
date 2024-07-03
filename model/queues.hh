#include <chrono>
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
#include <limits>
#include <random>
#include <iostream>
#include <cassert>
#include <unordered_map>

#define MAX_PACKET 1500


class Queue {
public:
    // Time till next event
    uint64_t buff = 0;

    // Statistics of individual slots
    struct slotstat_t {
	uint64_t occupied        = 0;  // # of cycles a valid element is in this slot
	uint64_t arrivals        = 0;  // # of cycles an arrival is put in this slot
	uint64_t arrivalbacklog  = 0;  // Sum of the backlog when messages arrive
    } slotstats[1024];
    
    // Statistics of the queue
    struct queuestat_t {
	uint64_t highwater = 0; // # of times highwater mark triggered
	uint64_t lowwater  = 0; // # of times lowwater mark triggered
	uint64_t maxsize   = 0; // Maximum occupancy of the queue
    } queuestats;

    // Statistics of the simulation
    struct simstat_t {
	uint64_t packets   = 0; // # of frames
	uint64_t comps     = 0; // # of completions
	uint64_t comptimes = 0; // Sum of completition times
	uint64_t slowdowns = 0; // Sum of slowdowns
	uint64_t underflow = 0; // Cycles of wasted network bandwidth
	uint64_t cycles    = 0; // Total number of simulation cycles
	uint64_t inacc     = 0; // Inaccuracies
    } simstats;

    struct entry_t {
	uint64_t ts;   // Start time
	int length;    // Original Length
	int remaining; // Remaining length
	bool taint;    // Has this message had inversions
    };

    // TODO move this inside entry
    // Return false reflexively if they are equal
    struct Compare {
	// TODO cleanup
	bool operator()(const entry_t & l, const entry_t & r) const {
	    if (l.remaining < r.remaining) {
		return true;
	    } else if (l.remaining == r.remaining && l.ts < r.ts) {
		return true;
	    } else if (l.remaining == r.remaining && l.ts == r.ts && l.length < r.length) {
		return true;
	    }
	    return false;
	}
    };

    std::string tracefile;

    std::multiset<entry_t, Compare> gqueue;

    Queue(std::string & tracefile) : tracefile(tracefile) {}

    ~Queue() {
	stats();
	dump();
    }

    virtual void step(uint64_t ts) = 0;
    virtual bool empty() = 0;

    virtual void arrival(uint64_t ts, int length) {
	entry_t entry{ts, length, length, false};
	gqueue.insert(entry);
	enqueue(entry);
    }

    virtual void departure(uint64_t ts) {
	simstats.cycles = ts;
	// Service node still processing last packet
	if (buff > 0) {
	    buff--;
	}

	if (buff != 0) {
	    return;
	}

	// If there are no elements to send, don't dequeue
	if (empty()) {
	    return;
	}

	entry_t entry = dequeue();

	// If the null entry is returned we have wasted bandwidth
	if (entry.length == 0) {
	    simstats.underflow++;
	    return;
	}

	// Otherwise a new packet has gone out 
	simstats.packets++;

	// If this message is not optimal, then mark it as tainted
	if (entry.remaining > (*gqueue.begin()).remaining) {
	    entry.taint = true;
	}

	// If we just sent the last packet for this message, record stats
	if (entry.remaining - std::ceil(((double) MAX_PACKET)/64.0) <= 0) {
	    simstats.comptimes += (ts - entry.ts);
	    simstats.slowdowns += (ts - entry.ts)/entry.length;
	    simstats.comps++;

	    if (entry.taint) {
		simstats.inacc++;
	    }

	    gqueue.erase(entry);
	} else {
	    gqueue.erase(entry);
	    entry.remaining -= std::ceil(((double) MAX_PACKET)/64.0);
	    gqueue.insert(entry);
	}
    }

    virtual void enqueue(entry_t) = 0;
    virtual entry_t dequeue() = 0;

    void stats() {
	// TODO def these inside structs
	std::cerr << "- Queue Statistics:" << std::endl;
	std::cerr << "  Highwater : " << queuestats.highwater << std::endl;
	std::cerr << "  Lowater   : " << queuestats.lowwater << std::endl;
	std::cerr << "  Max Size  : " << queuestats.maxsize << std::endl;
	std::cerr << "- Simulation Statistics:" << std::endl;
	std::cerr << "  Total Completions : " << simstats.comps << std::endl;
	std::cerr << "  Mean Completion   : " << ((double) simstats.comptimes) / simstats.comps << std::endl;
	std::cerr << "  Total Inacc       : " << simstats.inacc << std::endl;
	std::cerr << "  Inaccuracy Ratio  : " << ((double) simstats.inacc) / simstats.comps << std::endl;
	std::cerr << "  Mean Slowdown     : " << ((double) simstats.slowdowns) / simstats.comps << std::endl;
	std::cerr << "  Underflow         : " << simstats.underflow << std::endl;
	std::cerr << "  Underflow  Ratio  : " << ((double) simstats.underflow) / simstats.cycles << std::endl;
    }

    void dump() {
	int fd = open(tracefile.append(".stats").c_str(), O_RDWR | O_CREAT, 0644);
	write(fd, &queuestats, sizeof(queuestats));
	write(fd, &simstats, sizeof(simstats));
	write(fd, slotstats, 1024 * sizeof(*slotstats));
	close(fd);
     }
};

class Ideal : public Queue {
    std::multiset<entry_t, Compare> hwqueue;

public:
    Ideal(std::string & tracefile) : Queue(tracefile) {}
    ~Ideal() {}

    void enqueue(entry_t entry) override {
	auto it = hwqueue.insert(entry);

	int offset = std::distance(hwqueue.begin(), it);

	uint64_t mass = buff;
	int i = 0;
	for(const auto & it : hwqueue) {
	    if (i == offset) {
		break;
	    }
	    mass += it.remaining; 
	    i++;
	}
	
	slotstats[offset].arrivalbacklog += mass;
	slotstats[offset].arrivals += 1;
    }

    entry_t dequeue() override {
	entry_t ret = entry_t{0,0,0};
	if (!hwqueue.empty()) {
	    // Update to the current timestep
	    entry_t head = *hwqueue.begin();
	    ret = head;
	    head.remaining -= std::ceil(((double) MAX_PACKET)/64.0);
	    hwqueue.erase(hwqueue.begin());

	    if (head.remaining > 0) {
	    	// We still have more bytes to send for this message
	    	buff = std::ceil(((double) MAX_PACKET)/64.0);
		hwqueue.insert(head);
	    } else {
	    	// We sent this complete message
	    	buff = head.remaining + std::ceil(((double) MAX_PACKET)/64.0); 
	    }
	}

	return ret;
    }

    bool empty() override {
	return hwqueue.empty();
    }

    void step(uint64_t ts) override {
	if (queuestats.maxsize == 0 || hwqueue.size() > queuestats.maxsize) {
	    queuestats.maxsize = hwqueue.size();
	}

	int i = 0;
	for(const auto & it : hwqueue) {
	    slotstats[i].occupied += 1;
	    i++;
	}
    }
};


class PIFO_Naive: public Queue {
    std::multiset<entry_t, Compare> hwqueue;
    std::vector<entry_t> backing;

    int size = 0;
    int min_index = 0;
    int search = 0;
    int priorities;
    bool recsnap = false;

public:
    PIFO_Naive(int priorities, std::string & tracefile) : priorities(priorities), Queue(tracefile) {
	recsnap = true;
    }

    PIFO_Naive(int priorities, std::string & snapshot, std::string & tracefile) : priorities(priorities), Queue(tracefile) {
	std::cerr << snapshot << std::endl;
	int fd = open(snapshot.c_str(), O_RDWR, 0);
	if (fd == -1) {
	    perror("Inavlid snapshot file");
	    exit(EXIT_FAILURE);
	}

	uint32_t hwqsize;
	uint32_t bksize;
	read(fd, &hwqsize, sizeof(uint32_t));
	std::cerr << "hwqsize " << hwqsize << std::endl;
	read(fd, &bksize, sizeof(uint32_t));
	std::cerr << "bksize " << bksize << std::endl;
	entry_t entry;
	for (int i = 0; i < hwqsize; ++i) {
	    read(fd, &entry, sizeof(entry_t));
	    std::cerr << "REM " << entry.remaining << std::endl;
	    hwqueue.insert(entry);
	    size++;
	}

	while (read(fd, &entry, sizeof(entry_t)) != 0) {
	    std::cerr << "REM " << entry.remaining << std::endl;
	    backing.push_back(entry);
	    size++;
	}

	close(fd);
    }

    ~PIFO_Naive() {}

    void enqueue(entry_t entry) override {
	auto it = hwqueue.insert(entry);
	size++;
    }

    entry_t dequeue() override {
	entry_t ret = entry_t{0,0,0};
	if (!hwqueue.empty()) {
	    // Update to the current timestep
	    entry_t head = *hwqueue.begin();
	    ret = head;
	    head.remaining -= std::ceil(((double) MAX_PACKET)/64.0);
	    hwqueue.erase(hwqueue.begin());

	    if (head.remaining > 0) {
	    	// We still have more bytes to send for this message
	    	buff = std::ceil(((double) MAX_PACKET)/64.0);
		hwqueue.insert(head);
	    } else {
	    	// We sent this complete message
	    	buff = head.remaining + std::ceil(((double) MAX_PACKET)/64.0); // TODO move this?

		size--;
	    }
	}

	return ret;
    }

    bool empty() override {
	return size == 0;
    }

    void step(uint64_t ts) override {
	if (queuestats.maxsize == 0 || size > queuestats.maxsize) {
	    queuestats.maxsize = size;
	}

	if (!backing.empty()) {
	    if (search == backing.size()) {
		// We reached the end, return the best we found
		if (hwqueue.size() <= priorities + 1 - 2) {
		    entry_t best = backing[min_index];
		    backing.erase(std::next(backing.begin(), min_index));
		    hwqueue.insert(best);
		    search = 0;
		    min_index = 0;
		}
	    } else {
		// Either iterate search, or reset it
		entry_t candidate = backing[search];

		if (candidate.remaining <= backing[min_index].remaining) {
		    min_index = search;
		}
		search++;
	    }
	}

	// If there are too many entries in small queue, move them up
	if (hwqueue.size() >= priorities+1) {
	    queuestats.highwater++;

	    entry_t tail = *std::prev(hwqueue.end());
	    hwqueue.erase(std::prev(hwqueue.end()));

	    backing.push_back(tail);
	}

	if (!backing.empty()) {
	    slotstats[0].occupied += 1;
	    slotstats[0].arrivals += (buff != 0) ? 1 : 0;
	    for(int i = 0; i < priorities; ++i) {
		slotstats[i+1].occupied += 1;
		slotstats[i+1].arrivals += (i < hwqueue.size()) ? 1 : 0;
	    }
	}

	if (ts % 10000 == 0) {
	    std::string hwsnap = tracefile;
	    int fd = open((hwsnap + "_" + std::to_string(ts) + ".snap").c_str(), O_RDWR | O_CREAT, 0644);
	    uint32_t hwqsize = hwqueue.size();
	    uint32_t bkqsize = backing.size();

	    write(fd, &hwqsize, sizeof(uint32_t));
	    write(fd, &bkqsize, sizeof(uint32_t));

	    for(const auto & it : hwqueue) {
		write(fd, &it, sizeof(entry_t));
	    }

	    for(const auto & it : backing) {
		write(fd, &it, sizeof(entry_t));
	    }
	    close(fd);
	}
    }
};


 
