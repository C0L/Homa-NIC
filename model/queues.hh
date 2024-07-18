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
    uint64_t id   = 0;

    bool gstats = true;

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
	uint64_t packets   = 0;  // # of frames
	uint64_t comps     = 0;  // # of completions
	uint64_t comptimes = 0;  // Sum of completition times
	uint64_t slowdowns = 0;  // Sum of slowdowns
	double   violations = 0; // Sum of violations
	uint64_t presorted = 0;  // Sum # sorted entries at drain point
	uint64_t pulses    = 0;
	uint64_t underflow = 0;  // Cycles of wasted network bandwidth
	uint64_t cycles    = 0;  // Total number of simulation cycles
	uint64_t msginacc  = 0;  // Message inaccuracies
	uint64_t pktinacc  = 0;  // Packet inaccuracies
    } simstats;

    struct entry_t {
	uint64_t ts;   // Start time
	uint64_t id;   // Identifier
	int length;    // Original Length
	int remaining; // Remaining length
	bool taint;    // Has this message had inversions
    };

    std::vector<double> violations;

    // TODO move this inside entry
    // Return false reflexively if they are equal
    struct Compare {
	// TODO cleanup
	bool operator()(const entry_t & l, const entry_t & r) const {
	    // Primarily ordered by remianing bytes 
	    if (l.remaining < r.remaining) {
	    	return true;
	    } else if (l.remaining == r.remaining && l.id < r.id) {
		return true;
	    }

	    return false;
	}
    };

    std::string tracefile;

    std::multiset<entry_t, Compare> gqueue;

    int fd;

    Queue(std::string & tracefile) : tracefile(tracefile) {
	fd = open(tracefile.c_str(), O_RDWR | O_CREAT, 0644);
    }

    ~Queue() {
	stats();
	dump();
	close(fd);
    }

    virtual void step(uint64_t ts) = 0;
    virtual bool empty() = 0;

    virtual void arrival(uint64_t ts, int length) {
	entry_t entry{ts, id++, length, length, false};
	assert(length != 0);
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

	// if (!gstats) return;

	// If the null entry is returned we have wasted bandwidth
	if (entry.length == 0) {
	    if (gstats) simstats.underflow++;
	    return;
	}

	// Otherwise a new packet has gone out 
	if (gstats) simstats.packets++;

	// If this message is not optimal, then mark it as tainted
	if (entry.remaining > (*gqueue.begin()).remaining) {
	    if (gstats) simstats.pktinacc++;
	    double violation = (((double) entry.remaining)/((double) (*gqueue.begin()).remaining));
	    if (gstats) simstats.violations += violation;
	    if (gstats) violations.push_back(violation);
	    // TODO write to output here
	}

	// If we just sent the last packet for this message, record stats
	if (entry.remaining - std::ceil(((double) MAX_PACKET)/64.0) <= 0) {
	    if (gstats) simstats.comptimes += (ts - entry.ts);

	    if (gstats) simstats.slowdowns += (ts - entry.ts)/entry.length;
	    if (gstats) simstats.comps++;

	    // if (entry.taint) {
	    // 	simstats.msginacc++;
	    // }

	    int val = gqueue.erase(entry);
	    assert(val == 1);
	} else {
	    int val = gqueue.erase(entry);
	    assert(val == 1);
	    entry.remaining -= std::ceil(((double) MAX_PACKET)/64.0);
	    gqueue.insert(entry);
	}
    }

    virtual void enqueue(entry_t) = 0;
    virtual entry_t dequeue() = 0;
    virtual int size() = 0;
    virtual int backsize() = 0;

    void stats() {
	// TODO def these inside structs
	std::cerr << "- Queue Statistics:" << std::endl;
	std::cerr << "  Highwater : " << queuestats.highwater << std::endl;
	std::cerr << "  Lowater   : " << queuestats.lowwater << std::endl;
	std::cerr << "  Max Size  : " << queuestats.maxsize << std::endl;
	std::cerr << "- Simulation Statistics:" << std::endl;
	std::cerr << "  Total Completions : " << simstats.comps << std::endl;
	std::cerr << "  Mean Completion   : " << ((double) simstats.comptimes) / simstats.comps << std::endl;
	std::cerr << "  Total Inacc       : " << simstats.pktinacc << std::endl;
	std::cerr << "  Inaccuracy Ratio  : " << ((double) simstats.pktinacc) / simstats.packets << std::endl;
	std::cerr << "  Mean Slowdown     : " << ((double) simstats.slowdowns) / simstats.comps << std::endl;
	std::cerr << "  Underflow         : " << simstats.underflow << std::endl;
	std::cerr << "  Underflow  Ratio  : " << ((double) simstats.underflow) / simstats.cycles << std::endl;
    }

    void dump() {
	write(fd, &queuestats, sizeof(queuestats));
	write(fd, &simstats, sizeof(simstats));
	// write(fd, slotstats, 1024 * sizeof(*slotstats));
	write(fd, &(*violations.begin()), violations.size() * sizeof(double));
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
	entry_t ret = entry_t{0,0,0,0,false};
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

    int backsize() override {
	return 0;
    }

    int size() override {
	return hwqueue.size();
    }
};


class PIFO_Naive: public Queue {
    std::multiset<entry_t, Compare> hwqueue;
    std::vector<entry_t> insertion;
    std::vector<entry_t> backing;

    std::vector<entry_t> backing0;
    std::vector<entry_t> backing1;

    std::vector<entry_t> * primary   = &backing0;
    std::vector<entry_t> * secondary = &backing1;

    entry_t scan;

    int active = 0;
    // int min_index = 0;
    int search = 0;
    int priorities;
    // bool snapshot = false;
    // uint32_t last_snap = 0;
    // uint32_t snapc = 0;

    Compare cmp;

public:
    PIFO_Naive(int priorities, std::string & tfile) : priorities(priorities), Queue(tfile) {
	// snapshot = true;
    }

    PIFO_Naive(int priorities, std::string & sfile, std::string & tfile) : priorities(priorities), Queue(tfile) {
	// snapshot = false;
	int fd = open(sfile.c_str(), O_RDWR, 0);
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
	    hwqueue.insert(entry);
	    gqueue.insert(entry);
	    active++;
	}

	while (read(fd, &entry, sizeof(entry_t)) != 0) {
	    backing.push_back(entry);
	    gqueue.insert(entry);
	    active++;
	}

	close(fd);
    }

    ~PIFO_Naive() {}

    void enqueue(entry_t entry) override {
	auto it = hwqueue.insert(entry);
	active++;
    }

    entry_t dequeue() override {
	entry_t ret = entry_t{0,0,0,0,false};
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

		active--;
	    }
	}

	return ret;
    }

    bool empty() override {
	return active == 0;
    }

    void step(uint64_t ts) override {
	if (queuestats.maxsize == 0 || active > queuestats.maxsize) {
	    queuestats.maxsize = active;
	}

	if (!insertion.empty() && search == 0) {
	    // Get the top element of insertion queue
	    scan = *(insertion.begin());
	    insertion.erase(insertion.begin());

	    std::vector<entry_t> * tmp = secondary;
	    secondary = primary;
	    primary = tmp;

	    assert(primary->empty());

	    // First entry to insert into primary queue
	    entry_t ins;
	    if (secondary->empty()) {
		ins = scan;
	    } else {
		entry_t head = *(secondary->begin());
		secondary->erase(secondary->begin());
		ins  = cmp(head, scan) ? head : scan;
		scan = cmp(head, scan) ? scan : head;
		search++;
	    }

	    primary->push_back(ins);
	} else if (search != 0) {
	    entry_t ins;
	    if (secondary->empty()) {
		ins = scan;
		search = 0;
	    } else {
		entry_t head = *(secondary->begin());
		secondary->erase(secondary->begin());
		ins  = cmp(head, scan) ? head : scan;
		scan = cmp(head, scan) ? scan : head;
		search++;
		// TODO reset search=0 here?
	    }

	    primary->push_back(ins);
	}
	
	if (!primary->empty() && (hwqueue.size() <= priorities+1-8 || cmp(primary->front(), *std::next(hwqueue.begin(), priorities-8)))) {
	    entry_t head = *primary->begin();
	    primary->erase(primary->begin());
	    hwqueue.insert(head);
	}

	// If there are too many entries in small queue, move them up
	while (hwqueue.size() >= priorities+1) {
	    queuestats.highwater++;

	    entry_t tail = *std::prev(hwqueue.end());
	    hwqueue.erase(std::prev(hwqueue.end()));

	    // Move to insertion FIFO
	    insertion.push_back(tail);
	}
    }

    int backsize() override {
	return primary->size() + secondary->size() + 1;
    }

    int size() override {
	return active;
    }
};
