#include "config.hh"

using namespace std;

const struct option longOpts[] {
    {"workload",required_argument,NULL,'w'},
    {"comps",required_argument,NULL,'c'},
    {"utilization",required_argument,NULL,'u'},
    {"trace-file",required_argument,NULL,'t'},
};

int main(int argc, char ** argv) {
    int c;

    opterr = 0;

    uint64_t comps;
    double utilization;
    char * end;
    std::string wfile;
    std::string tfile;

    while ((c = getopt_long(argc, argv, "w:u:c:t:", longOpts, NULL)) != -1)
	switch (c) {
	    case 'w':
		wfile = std::string(optarg);
		break;
	    case 'u':
		utilization = strtod(optarg, &end);
		break;
	    case 'c':
		comps = strtol(optarg, &end, 10);
		break;
	    case 't':
		tfile = std::string(optarg);
		break;
	    default:
		abort();
	}

    std::cerr << "Launching Simulator:" << std::endl;
    std::cerr << "  - Workload      : " << wfile << std::endl;
    std::cerr << "  - Utilization   : " << utilization << std::endl;
    std::cerr << "  - Completions   : " << comps << std::endl;
    std::cerr << "  - Trace File    : " << tfile << std::endl;

    // std::mt19937 gen{rand()};
    // std::mt19937 gen(std::chrono::high_resolution_clock::now().time_since_epoch().count()); // time seed
    std::mt19937 gen{0xdeadbeef}; 

    dist_point_gen wk(wfile.c_str(), 1000000);

    // Compute the desired arrival cycles 
    double desired_arrival = wk.get_mean() * (1.0/utilization);

    // Allocate space for statistics about slots
    slotstat_t * slotstats = (slotstat_t *) calloc(262144, sizeof(slotstat_t));

    // Interarrival process with desired mean
    std::poisson_distribution<int> arrivals(desired_arrival);

    // Initialize arrival and length
    uint64_t arrival = arrivals(gen);
    int length = std::ceil(wk(gen)/64.0);

    // Simulation queue
    std::multiset<entry_t, Compare> hwqueue;

    // Simulation time
    uint64_t ts = 0;

    // Time till next event
    uint32_t buff = 0;

    // Iterate through events until we reach the requisite number of completitions
    for (;simstat.comps < comps || !hwqueue.empty();) {
	// Determine if we have reached a new maximum occupancy
	/*
	if (hwqueue.size() > simstat.max) {
	    simstat.max = hwqueue.size();
	    std::cerr << "New Max at time: " << ts << " of size " << hwqueue.size() << std::endl;

	    uint64_t mass = 0;
	    uint32_t i = 0;

	    std::cerr << "Index     |";
	    for(const auto & it : hwqueue) {
		fprintf(stderr, " %8d |", i);
		i++;
	    }
	    std::cerr << std::endl;

	    std::cerr << "Birth     |";
	    for(const auto & it : hwqueue) {
		fprintf(stderr, " %8d |", it.ts);
	    }
	    std::cerr << std::endl;

	    std::cerr << "Remaining |";
	    for(const auto & it : hwqueue) {
		fprintf(stderr, " %8d |", it.remaining);
	    }
	    std::cerr << std::endl;

	    std::cerr << "Length    |";
	    for(const auto & it : hwqueue) {
		fprintf(stderr, " %8d |", it.length);
	    }
	    std::cerr << std::endl;
	}
	*/

	// Each cycle we send 64 bytes
	if (buff > 0) {
	    buff -= 1;
	}

	// 16 bytes in head remaining
	if (!hwqueue.empty() && buff == 0) {
	    // Update to the current timestep
	    entry_t head = *hwqueue.begin(); 
	    hwqueue.erase(hwqueue.begin());
	    head.remaining -= std::ceil(((double) MAX_PACKET)/64.0);

	    if (head.remaining > 0) {
		// We still have more bytes to send for this message
		hwqueue.insert(head);

		buff = std::ceil(((double) MAX_PACKET)/64.0);
	    } else {
		// We sent this complete message
		buff = head.remaining + std::ceil(((double) MAX_PACKET)/64.0);
		// simstat.events++;
		simstat.sum_comps += (ts - head.ts);
		simstat.sum_slow_down += (ts - head.ts)/head.length;
		simstat.comps++;
	    }
	} 

	// If the next arrival time is before or during this current timestep then add it to the queue
	// if (arrival <= ts) {
	if (arrival <= ts && simstat.comps < comps) {
	    entry_t ins = entry_t{ts, length, length};

	    assert(length != 0 && "initial length should not be 0");
	    hwqueue.insert(ins);

	    // simstat.events++;

	    // Move to the next arrival and length
	    arrival += arrivals(gen);
	    length  = std::ceil(wk(gen)/64.0);
	}

	// next_event = min({
	// 	(!hwqueue.empty() ? (uint64_t) std::ceil((hwqueue.begin())->remaining / 64.0) : (uint64_t) -1),
	// 	(uint64_t) (std::floor(arrival / 64.0) - ts)
	//    });

	// assert(next_event != 0 && "next_event should never be zero");

	uint64_t mass = 0;
	uint32_t i = 0;

	for(const auto & it : hwqueue) {
	    mass += it.remaining; 
	    slotstats[i].validcycles += 1;
	    slotstats[i].totalbacklog += mass * 1;

	    slotstats[i].backlog += it.remaining * 1;
	    
	    if (slotstats[i].minbacklog > it.remaining || slotstats[i].minbacklog == 0) {
		slotstats[i].minbacklog = it.remaining;
	    }
	    
	    if (slotstats[i].mintotalbacklog > mass || slotstats[i].mintotalbacklog == 0) {
		slotstats[i].mintotalbacklog = mass;
	    }
	    
	    i++;
	}

	// Move the timestep to the next event
	ts += 1;
    }

    simstat.cycles = ts;

    // std::cerr << "Experimental Load : " << ((double)bytes_out) / ts << std::endl;

    // std::cerr << "Max Size: " << max_size << std::endl;
    std::cerr << "Total Completion : " << simstat.comps << std::endl;
    std::cerr << "Mean Completion  : " << ((double) simstat.sum_comps) / simstat.comps << std::endl;
    std::cerr << "Inaccuracies     : " << ((double) simstat.inacc) / simstat.cycles << std::endl;
    std::cerr << "Mean Slowdown    : " << ((double) simstat.sum_slow_down) / simstat.comps << std::endl;

    // std::cerr << "Total Cmpl: " << simstat.comps << std::endl;
    // std::cerr << "Mean Cmpl: " << simstat.sum_comps / simstat.comps << std::endl;
    // std::cerr << "Final Size: " << hwqueue.size() << std::endl;
    // std::cerr << "Average Size: " << simstat.qo / simstat.cycles << std::endl;
    // std::cerr << "Max Size: " << simstat.max << std::endl;

    // for (int i = 0; i < 64; i++) {
    // 	std::cerr << i << " VALID COMPS " << slotstats[i].validcycles << " " << slotstats[i].backlog <<std::endl;
    // }

    string slotstatroot = tfile;
    int fslotstat = open(slotstatroot.append(".slotstats").c_str(), O_RDWR | O_CREAT, 0644);
    write(fslotstat, &simstat, sizeof(simstat_t));
    write(fslotstat, slotstats, 262144 * sizeof(slotstat_t));
    close(fslotstat);
}
