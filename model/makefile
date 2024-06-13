.PHONY: dist
dist:
	g++ -c dist.cc

.PHONY: distgen
distgen:
	g++ -c dist.cc
	g++ -Idist.hh distgen.cc dist.o -o distgen

.PHONY: simulator
simulator:
	g++ simulator.cc -o simulator

.PHONY: sim_pifo_logn
sim_pifo_logn:
	g++ -std=c++11 sim_pifo_logn.cc dist.o -o sim_pifo_logn 

.PHONY: sim_pifo_lin
sim_pifo_lin:
	g++ -std=c++11 sim_pifo_lin.cc dist.o -o sim_pifo_lin

.PHONY: sim_ideal
sim_ideal:
	g++ -std=c++11 sim_ideal.cc -o sim_ideal

.PHONY: sim_poisson_ideal
sim_poisson_ideal:
	g++ -std=c++11 sim_poisson_ideal.cc dist.o -o sim_poisson_ideal

.PHONY: sim_weibull_ideal
sim_weibull_ideal:
	g++ -std=c++11 sim_weibull_ideal.cc dist.o -o sim_weibull_ideal

.PHONY: sim_poisson_hw_model
sim_poisson_hw_model:
	g++ -std=c++11 sim_poisson_hw_model.cc dist.o -o sim_poisson_hw_model

.PHONY: sim_poisson_strict
sim_poisson_strict:
	g++ -std=c++11 sim_poisson_strict.cc dist.o -o sim_poisson_strict

queueprof: clean simulator distgen
	./queueprof.sh

lomax_slot_profile: sim_ideal distgen
	./lomax_slot_profile.sh

weibull_slot_profile: sim_weibull_ideal distgen
	./weibull_slot_profile.sh

poisson_slot_profile: dist sim_poisson_ideal 
	./poisson_slot_profile.sh

poisson_hw_model: dist sim_poisson_hw_model
	./poisson_hw_model.sh

poisson_strict: dist sim_poisson_strict
	./poisson_strict.sh

pareto_efficient_weibull_pifo_logn: sim_pifo_logn distgen
	./pareto_efficient_weibull_logn.sh

pareto_efficient_weibull_pifo_lin: sim_pifo_lin distgen
	./pareto_efficient_weibull_pifo_lin.sh

clean:
	rm -rf simulator
	rm -rf distgen
#  rm -rf dists/*
	rm -rf traces/*
