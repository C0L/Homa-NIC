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

.PHONY: sim_ideal
sim_ideal:
	g++ -std=c++11 sim_ideal.cc -o sim_ideal

queueprof: clean simulator distgen
	./queueprof.sh

lomax_slot_profile: sim_ideal distgen
	./lomax_slot_profile.sh

poisson_slot_profile: sim_ideal distgen
	./poisson_slot_profile.sh

pareto_efficient_lomax_pifo_logn: sim_pifo_logn distgen
	./pareto_efficient_lomax_pifo_logn.sh

pareto_efficient_lomax_pifo_lin: sim_pifo_lin distgen
	./pareto_efficient_lomax_pifo_lin.sh

clean:
	rm -rf simulator
	rm -rf distgen
#  rm -rf dists/*
	rm -rf traces/*
