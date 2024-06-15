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

.PHONY: sim_strict_priorities
sim_strict_priorities:
	g++ -std=c++11 -I./config.hh sim_strict_priorities.cc dist.o -o sim_strict_priorities

.PHONY: sim_poisson_strict_log
sim_poisson_strict_log:
	g++ -std=c++11 sim_poisson_strict_log.cc dist.o -Iconfig.hh -o sim_poisson_strict_log

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

strict_priorities: dist sim_strict_priorities
	./strict_priorities.sh

poisson_strict_log: dist sim_poisson_strict_log
	./poisson_strict_log.sh

pareto_efficient_weibull_pifo_logn: sim_pifo_logn distgen
	./pareto_efficient_weibull_logn.sh

pareto_efficient_weibull_pifo_lin: sim_pifo_lin distgen
	./pareto_efficient_weibull_pifo_lin.sh

clean:
	rm -rf simulator
	rm -rf distgen
#  rm -rf dists/*
	rm -rf traces/*
