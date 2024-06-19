#include "config.hh"

const struct option longOpts[] {
    {"workload",required_argument,NULL,'w'},
    {"comps",required_argument,NULL,'c'},
    {"priorities",required_argument,NULL,'p'},
    {"utilization",required_argument,NULL,'u'},
    {"trace-file",required_argument,NULL,'t'},
};

int main(int argc, char ** argv) {
    int c;

    opterr = 0;

    uint64_t comps;
    uint32_t priorities;
    double utilization;
    char * end;
    std::string wfile;
    std::string tfile;

    while ((c = getopt_long(argc, argv, "w:u:c:t:p:", longOpts, NULL)) != -1)
	switch (c) {
	    case 'w':
		wfile = std::string(optarg);
		break;
	    case 'u':
		utilization = strtod(optarg, &end);
		break;
	    case 'p':
		priorities = strtol(optarg, &end, 10);
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
    std::cerr << "  - Priorities    : " << priorities << std::endl;
    std::cerr << "  - Completions   : " << comps << std::endl;
    std::cerr << "  - Trace File    : " << tfile << std::endl;

    std::vector<entry_t> * fifos = new std::vector<entry_t>[priorities];

    std::mt19937 gen{0xdeadbeef}; 

    // Message distribution capped at 1MB
    //dist_point_gen w1("w1", 1000000);
    // dist_point_gen w2("w2", 1000000);
    // dist_point_gen w3("w3", 1000000);
    // dist_point_gen w4("w4", 1000000);
    // dist_point_gen w5("w5", 1000000);

    dist_point_gen wk(wfile.c_str(), 1000000);

    // Compute the desired arrival cycles 
    double desired_arrival = wk.get_mean() * (1.0/utilization);

    // Interarrival process with desired mean
    std::poisson_distribution<int> arrivals(desired_arrival);

    // double B = desired_arrival / std::tgamma((1+1/.1));
    // std::weibull_distribution<> arrivals(.1, B);

    // Initialize arrival and length
    uint64_t arrival = std::round(arrivals(gen));

    // int fifo_depth = std::ceil((double) TOTAL_SLOTS / priorities);
    int fifo_depths[priorities];

    // Inital resource alloc
    for (int i = 0; i < priorities; ++i) {
	fifo_depths[i] = TOTAL_SLOTS / priorities;
    }

    // Remaining resource alloc
    for (int i = 0; i < (TOTAL_SLOTS - (priorities * std::floor(TOTAL_SLOTS / priorities))); ++i) {
	fifo_depths[i%priorities]++;
    }

    int length = std::ceil(wk(gen)/64.0);

    // Golden queue 
    std::multiset<entry_t, Compare> gqueue;
    std::vector<entry_t> rexmit;

    // Simulation queue

    uint64_t size = 0;

    std::vector<entry_t> backing;

    // Simulation time
    uint64_t ts = 0;

    // If priorities increases, bucket_size should decrease
    double bucket_size = ((double) priorities-1) / std::log2(std::ceil(1000000/64.0));

    uint32_t buff = 0;

    uint32_t max_size = 0;

    // Iterate through events until we reach the requisite number of completitions
    for (;simstat.comps < comps || size != 0;) {

	assert(rexmit.size() == 0);

	// simstat.max = (size > simstat.max) ? size : max_size;

	// Each cycle we send 64 bytes
	if (buff > 0) {
	    buff -= 1;
	}

	// If the small queue is not empty, decrement the head or pop it
	if (size != 0 && buff == 0) {
	    int fifo = 0;
	    for (; fifo < priorities; ++fifo) {
		if (!fifos[fifo].empty()) break;
	    }

	    entry_t head = fifos[fifo].front();
	    fifos[fifo].erase(fifos[fifo].begin());

	    assert(head.remaining >= (*gqueue.begin()).remaining);
	    if (head.remaining > (*gqueue.begin()).remaining) {
		simstat.inacc++;
	    }

	    gqueue.erase(head);

	    head.remaining -= std::ceil(((double) MAX_PACKET)/64.0);
	    simstat.packets++;

	    if (head.remaining > 0) {
		int bucket = std::floor(std::log2(head.remaining) * bucket_size);
		fifos[bucket].insert(fifos[bucket].begin(), head);
		// fifos[fifo].insert(fifos[fifo].begin(), head);

		gqueue.insert(head);

		buff = std::ceil(((double) MAX_PACKET)/64.0);
	    } else {

		buff = head.remaining + std::ceil(((double) MAX_PACKET)/64.0);

		simstat.sum_comps += (ts - head.ts);
		simstat.sum_slow_down += (ts - head.ts)/head.length;
		simstat.comps++;
		size--;
	    }
	}

	// If the next arrival time is before or during this current timestep then add it to the queue
	if ((arrival <= ts && simstat.comps < comps) || !rexmit.empty()) {
	    int bucket;
	    entry_t ins;
	    if (arrival <= ts) {
		ins = entry_t{ts, length, length};
		
		assert(length != 0 && "initial length should not be 0");
		
		bucket = std::floor(std::log2(length) * bucket_size);

		// Move to the next arrival and length
		arrival += std::round(arrivals(gen));

		int new_length = std::ceil(wk(gen)/64.0);

		length = new_length;
	    } else if (!rexmit.empty()) {
		ins = rexmit.front();
		rexmit.erase(rexmit.begin());

		bucket = std::floor(std::log2(length) * bucket_size);
	    }

	    if (fifos[bucket].size() < fifo_depths[bucket]) {
		size++;
		gqueue.insert(ins);
		fifos[bucket].push_back(ins);
	    } else {
		rexmit.push_back(ins);
	    }
	}

	// Move the timestep to the next event
	ts += 1;
    }

    simstat.cycles = ts;

    std::cerr << "Max Size: " << max_size << std::endl;
    std::cerr << "Total Completion : " << simstat.comps << std::endl;
    std::cerr << "Mean Completion  : " << ((double) simstat.sum_comps) / simstat.comps << std::endl;
    std::cerr << "Inaccuracies     : " << ((double) simstat.inacc) / simstat.cycles << std::endl;
    std::cerr << "Mean Slowdown    : " << ((double) simstat.sum_slow_down) / simstat.comps << std::endl;

    std::string slotstatroot = tfile;
    int fslotstat = open(slotstatroot.append(".slotstats").c_str(), O_RDWR | O_CREAT, 0644);
    write(fslotstat, &simstat, sizeof(simstat_t));
    close(fslotstat);
}
