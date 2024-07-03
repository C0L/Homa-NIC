#include "queues.hh"
#include "dist.h"

const struct option longOpts[] {
    {"priorities",required_argument,NULL,'p'},
    {"type",required_argument,NULL,'q'},
    {"workload",required_argument,NULL,'w'},
    {"comps",required_argument,NULL,'c'},
    {"utilization",required_argument,NULL,'u'},
    {"trace-file",required_argument,NULL,'t'},
};

int main(int argc, char ** argv) {
    int c;

    uint64_t duration;
    double util;
    char * end;
    std::string qtype;
    std::string wfile;
    std::string tfile;
    int priorities = 0;

    while ((c = getopt_long(argc, argv, "p:q:w:u:c:t:", longOpts, NULL)) != -1)
	switch (c) {
	    case 'p':
		priorities = std::stoi(optarg);
		break;
	    case 'q':
		qtype = std::string(optarg);
		break;
	    case 'w':
		wfile = std::string(optarg);
		break;
	    case 'u':
		util = strtod(optarg, &end);
		break;
	    case 'c':
		duration = strtol(optarg, &end, 10);
		break;
	    case 't':
		tfile = std::string(optarg);
		break;
	    default:
		abort();
	}

    std::mt19937 gen{0xdeadbeef}; 

    dist_point_gen wk(wfile.c_str(), 1000000, 0);

    // Compute the desired arrival cycles 
    double desired_arrival = wk.get_mean()/util;

    // Interarrival process with desired mean
    std::exponential_distribution<double> arrivals(1.0/desired_arrival);

    Queue * queue;

    std::unordered_map<std::string, int> nmap = {
	{"ideal", 0},
	{"pifonaive", 1}
    };

    switch(nmap[qtype]) {
    	case 0:
    	    queue = new Ideal(tfile);
	    break;
    	case 1:
    	    queue = new PIFO_Naive(priorities, tfile);
	    break;
    }

    double next_arrival = arrivals(gen);
    int next_length = std::ceil(wk(gen)/64.0);

    // Simulation time
    uint64_t ts = 0;

    for (;queue->simstats.comps < duration;) {
	queue->departure(ts);

	// Is there a new arrival
	if (next_arrival <= ts) {
	    assert(next_length != 0 && "initial length should not be 0");

	    queue->arrival(ts, next_length);

	    // Move to the next arrival and length
	    next_arrival += arrivals(gen);
	    next_length  = std::ceil(wk(gen)/64.0);
	}

	queue->step();

	ts++;
    }

    delete queue;

    return 1;
}

