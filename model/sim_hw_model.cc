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

    std::mt19937 gen{0xdeadbeef}; 

    dist_point_gen wk(wfile.c_str(), 1000000);

    // Compute the desired arrival cycles 
    double desired_arrival = wk.get_mean() * (1.0/utilization);

    // Interarrival process with desired mean
    std::poisson_distribution<int> arrivals(desired_arrival);

    // double B = desired_arrival / std::tgamma((1+1/.1));
    // std::weibull_distribution<> arrivals(.1, B);

    // Initialize arrival and length
    uint64_t arrival = std::round(arrivals(gen));

    int length = std::ceil(wk(gen)/64.0);

    // Simulation queue
    std::multiset<entry_t, Compare> hwqueue;
    std::vector<entry_t> backing;

    // Golden queue 
    std::multiset<entry_t, Compare> gqueue;
    std::vector<entry_t> rexmit;

    // Simulation queue
    uint64_t size = 0;

    // Simulation time
    uint64_t ts = 0;

    uint32_t buff = 0;

    uint32_t min_index = 0;
    int search = 0;

    // Iterate through events until we reach the requisite number of completitions
    for (;simstat.comps < comps || !hwqueue.empty() || !backing.empty() || !rexmit.empty();) {
	// Each cycle we send 64 bytes
	if (buff > 0) {
	    buff -= 1;
	}

	// If the small queue is not empty, decrement the head or pop it
	if (!hwqueue.empty() && buff == 0) {
	    entry_t head = *hwqueue.begin();
	    hwqueue.erase(hwqueue.begin());

	    assert(head.remaining >= (*gqueue.begin()).remaining);
	    if (head.remaining > (*gqueue.begin()).remaining) {
		simstat.inacc++;
	    }

	    gqueue.erase(head);

	    head.remaining -= std::ceil(((double) MAX_PACKET)/64.0);
	    simstat.packets++;

	    if (head.remaining > 0) {
		hwqueue.insert(head);

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
	    
		// Move to the next arrival and length
		arrival += std::round(arrivals(gen));

		int new_length = std::ceil(wk(gen)/64.0);

		length  = new_length;

	    } else if (!rexmit.empty()) {
		ins = rexmit.front();
		rexmit.erase(rexmit.begin());
	    }

	    gqueue.insert(ins);
	    hwqueue.insert(ins);

	    size++;
	}


	// If there are too few entries in small queue start a new search
	if (hwqueue.size() <= priorities+1 - 2 && !backing.empty()) {
	    // Either iterate search, or reset it
	    entry_t candidate = backing[search];

	    if (candidate.remaining <= backing[min_index].remaining) {
		min_index = search;
	    }

	    search++;

	    // We reached the end, return the best we found
	    if (search == backing.size()) {
		simstat.lowwater++;
		entry_t best = backing[min_index];
		backing.erase(std::next(backing.begin(), min_index));
		hwqueue.insert(best);
		search = 0;
		min_index = 0;
	    }
	}

	// If there are too many entries in small queue, move them up
	if (hwqueue.size() >= priorities+1) {
	    simstat.highwater++;

	    entry_t tail = *std::prev(hwqueue.end());
	    hwqueue.erase(std::prev(hwqueue.end()));

	    // TODO temp
	    if (backing.size() < TOTAL_SLOTS - (priorities+1)) {
		backing.push_back(tail);
	    } else {
		rexmit.push_back(tail);
		gqueue.erase(tail);
		size--;
	    }
	}

	// Move the timestep to the next event
	ts += 1;
    }

    simstat.cycles = ts;

    // std::cerr << "Max Size: " << max_size << std::endl;
    std::cerr << "Total Completion : " << simstat.comps << std::endl;
    std::cerr << "Mean Completion  : " << ((double) simstat.sum_comps) / simstat.comps << std::endl;
    std::cerr << "Inaccuracies     : " << ((double) simstat.inacc) / simstat.cycles << std::endl;
    std::cerr << "Mean Slowdown    : " << ((double) simstat.sum_slow_down) / simstat.comps << std::endl;

    std::string slotstatroot = tfile;
    int fslotstat = open(slotstatroot.append(".slotstats").c_str(), O_RDWR | O_CREAT, 0644);
    write(fslotstat, &simstat, sizeof(simstat_t));
    close(fslotstat);
}



/*




    // Simulation time
    uint64_t ts = 0;

    uint32_t min_index = 0;
    uint32_t search = 0;

    // Iterate through events until we reach the requisite number of completitions
    for (;simstat.compcount < comps;) {

	uint64_t buff = 64;
	// If the small queue is not empty, decrement the head or pop it
	while (!hwqueue.empty() && buff != 0) {
	    entry_t head = *hwqueue.begin();
	    hwqueue.erase(hwqueue.begin());

	    if (head.ts != (*gqueue.begin()).ts) {
		simstat.inacc++;
	    }

	    gqueue.erase(head);

	    head.remaining -= buff;

	    if (head.remaining > 0) {
		hwqueue.insert(head);
		gqueue.insert(head);
		buff = 0;
	    } else {
			
		buff -= (buff + head.remaining);

		simstat.events++;
		simstat.compsum += (ts - head.ts);
		simstat.compcount++;

		// head.remaining = head.length;

		// std::cerr << "Trying to remove " << head << std::endl;

		// std::cerr << "hwqueue" << std::endl;
		// for (auto & it : hwqueue) {
		//     std::cerr << it << std::endl;
		// }

		// std::cerr << "Gqueue" << std::endl;
		// for (auto & it : gqueue) {
		//     std::cerr << it << std::endl;
		// }

		// auto it = gqueue.find(head);
		// assert(it != gqueue.end());
		// std::cerr << "Trying to remove " << head.remaining << " " << head.length << " " << head.ts << std::endl;
		// std::cerr << "Found            " << (*it).remaining << " " << (*it).length << " " << (*it).ts << std::endl;

		// int pre = gqueue.size();
		// gqueue.erase(head);
		// int post = gqueue.size();

		// assert(post<pre);
	    }
	}

	// If the next arrival time is before or during this current timestep then add it to the queue
	while (std::floor(arrival/64.0) <= ts) {
	    entry_t ins = entry_t{ts, length, length};

	    assert(length != 0 && "initial length should not be 0");
	    hwqueue.insert(ins);
	    gqueue.insert(ins);

	    simstat.events++;

	    // Move to the next arrival and length
	    arrival += arrivals(gen);
	    length  = generator(gen);
	}

	// Two lists

	// Start reading through one, and write to the other one as you go,
	// Always withhold the smallest value
	// When you find a new smallest value, write your previous one to list 2 instead of value just read

	// If there are too few entries in small queue start a new search
	if (hwqueue.size() <= 2 && !backing.empty()) {
	    // Either iterate search, or reset it
	    entry_t candidate = backing[search];

	    if (candidate.remaining <= backing[min_index].remaining) {
		min_index = search;
	    }

	    search++;

	    // We reached the end, return the best we found
	    if (search == backing.size()) {
		simstat.lowwater++;
		entry_t best = backing[min_index];
		backing.erase(std::next(backing.begin(), min_index));
		hwqueue.insert(best);
		search = 0;
		min_index = 0;
	    }
	}

	// If there are too many entries in small queue, move them up
	if (hwqueue.size() >= 4) {
	    simstat.highwater++;

	    entry_t tail = *std::prev(hwqueue.end());
	    hwqueue.erase(std::prev(hwqueue.end()));

	    backing.push_back(tail);
	}

	// Move the timestep to the next event
	ts += 1;
    }

    simstat.cycles = ts;

    std::cerr << "Total Cmpl: " << simstat.compcount << std::endl;
    std::cerr << "Mean Cmpl: " << ((double) simstat.compsum) / simstat.compcount << std::endl;
    std::cerr << "Highwaters: " << ((double) simstat.highwater)/simstat.cycles << std::endl;
    std::cerr << "Lowwaters: " << ((double) simstat.lowwater)/simstat.cycles << std::endl;
    std::cerr << "Final Size: " << hwqueue.size() << std::endl;
    std::cerr << "Average Size: " << simstat.qo / simstat.cycles << std::endl;
    std::cerr << "Max Size: " << simstat.max << std::endl;
    std::cerr << "Inaccuracies: " << ((double) simstat.inacc) / simstat.cycles << std::endl;

    // for (int i = 0; i < 64; i++) {
    // 	std::cerr << i << " VALID COMPS " << slotstats[i].validcycles << " " << slotstats[i].backlog <<std::endl;
    // }

    string slotstatroot = tfile;
    int fslotstat = open(slotstatroot.append(".slotstats").c_str(), O_RDWR | O_CREAT, 0644);
    write(fslotstat, &simstat, sizeof(simstat_t));
    close(fslotstat);
}
*/
