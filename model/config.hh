#ifndef CONFIG_H
#define CONFIG_H

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

#include "dist.h"

#define MAX_PACKET 1500
#define TOTAL_SLOTS 10000000000
// #define TOTAL_SLOTS 256

struct simstat_t {
    uint64_t highwater; // # of times highwater mark triggered
    uint64_t lowwater;  // # of times lowwater mark triggered
    uint64_t max;       // Maximum occupancy of the queue
    uint64_t packets;    // # of events (push/pop)
    uint64_t comps; // # of completions
    uint64_t sum_comps;   // Some of MCTs
    uint64_t cycles;    // Total number of simulation cycles
    uint64_t inacc;     // Inaccuracies
    uint64_t sum_slow_down;
};

struct slotstat_t {
    uint64_t validcycles;  // # of cycles a valid element is in this slot
    uint64_t totalbacklog; // Sum of mass of this slot and down
    uint64_t backlog;      // Sum of mass in this slot
    uint64_t mintotalbacklog; // Minimum total backlog
    uint64_t minbacklog;      // Minimum backlog
};

struct entry_t {
    uint64_t ts;   // Start time
    int length;    // Original Length
    int remaining; // Remaining length
};

std::ostream& operator<<(std::ostream& os, const entry_t& entry) {
    return os << entry.ts << " " << entry.length << " " << entry.remaining;
}

// Return false reflexively if they are equal
struct Compare {
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

static simstat_t simstat;

#endif
