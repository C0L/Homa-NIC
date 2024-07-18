.PHONY: dist
dist:
	g++ -c dist.cc

.PHONY: qsim 
qsim: dist
	g++ -std=c++11 qsim.cc dist.o -o qsim

prof_underflow: qsim
	./prof_underflow.sh

pifo_insertion_pulse: qsim
	./pifo_insertion_pulse.sh

prof_snapshots: qsim
	./prof_snapshots.sh

clean:
	rm -rf simulator
	rm -rf distgen
	rm -rf traces/*
