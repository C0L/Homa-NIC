.PHONY: distgen
distgen:
	g++ -c dist.cc
	g++ -Idist.hh distgen.cc dist.o -o distgen

.PHONY: simulator
simulator:
	g++ simulator.cc -o simulator

.PHONY: sim_pifo_logn
sim_pifo_logn:
	g++ -std=c++11 sim_pifo_logn.cc -o sim_pifo_logn 

.PHONY: sim_pifo_lin
sim_pifo_lin:
	g++ -std=c++11 sim_pifo_lin.cc -o sim_pifo_lin

queueprof: clean simulator distgen
	./queueprof.sh

# queuebisect: clean simulator distgen
# 	./queuebisect.sh

# queuestress: clean simulator distgen
# 	./queuestress.sh

lomaxprof: clean simulator distgen
	./lomaxprof.sh


lomaxpareto: clean simulator distgen
	./lomaxpareto.sh

pareto_efficient_lomax_pifo_logn: sim_pifo_logn distgen
	./pareto_efficient_lomax_pifo_logn.sh

pareto_efficient_lomax_pifo_lin: sim_pifo_lin distgen
	./pareto_efficient_lomax_pifo_lin.sh

clean:
	rm -rf simulator
	rm -rf distgen
#  rm -rf dists/*
	rm -rf traces/*
