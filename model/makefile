.PHONY: dist
dist:
	g++ -c dist.cc

.PHONY: distgen
distgen:
	g++ -c dist.cc
	g++ -Idist.hh distgen.cc dist.o -o distgen

.PHONY: sim_pifo_naive
sim_pifo_naive:
	g++ -std=c++11 sim_pifo_naive.cc dist.o -o sim_pifo_naive

.PHONY: sim_sp_naive
sim_sp_naive:
	g++ -std=c++11 sim_sp_naive.cc dist.o -o sim_sp_naive

.PHONY: sim_fifo_naive
sim_fifo_naive:
	g++ -std=c++11 sim_fifo_naive.cc dist.o -o sim_fifo_naive

hw_model: dist sim_hw_model
	./hw_model.sh

.PHONY: sim_ideal
sim_ideal:
	g++ -std=c++11 qsim.cc dist.o -o qsim

slot_profile: dist sim_ideal 
	./slot_profile.sh

.PHONY: sim_strict_priorities
sim_strict_priorities:
	 g++ -std=c++11 -I./config.hh sim_strict_priorities.cc dist.o -o sim_strict_priorities

strict_priorities: dist sim_ideal sim_strict_priorities
	./strict_priorities.sh

qperf: dist sim_fifo_naive sim_pifo_naive sim_strict_priorities sim_sp_naive
	./qperf.sh

clean:
	rm -rf simulator
	rm -rf distgen
	rm -rf traces/*
