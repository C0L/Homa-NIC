.PHONY: dist
dist:
	g++ -c dist.cc

.PHONY: distgen
distgen:
	g++ -c dist.cc
	g++ -Idist.hh distgen.cc dist.o -o distgen

.PHONY: sim_hw_model
sim_hw_model:
	g++ -std=c++11 sim_hw_model.cc dist.o -o sim_hw_model

hw_model: dist sim_hw_model
	./hw_model.sh

.PHONY: sim_ideal
sim_ideal:
	g++ -std=c++11 sim_ideal.cc dist.o -o sim_ideal

slot_profile: dist sim_ideal 
	./slot_profile.sh

.PHONY: sim_strict_priorities
sim_strict_priorities:
	 g++ -std=c++11 -I./config.hh sim_strict_priorities.cc dist.o -o sim_strict_priorities

strict_priorities: dist sim_ideal sim_strict_priorities
	./strict_priorities.sh

clean:
	rm -rf simulator
	rm -rf distgen
	rm -rf traces/*
