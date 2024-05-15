.PHONY: distgen
distgen:
	g++ -c dist.cc
	g++ -Idist.hh distgen.cc dist.o -o distgen

# %.dist: distgen
# 	- mkdir dists
# 	./distgen $(basename $@) 10000 > dists/$@

# %.arrival: %.dist
# 	for util in $(UTILS); do \
# 		python3 GenerateArrivals.py -d pareto -u $$util -w dists/$< -a dists/$(basename $@)_$$util.arrival -s 10000 ;\
# 	done

.PHONY: simulator
simulator:
	g++ simulator.cc -o simulator

# %.trace: %.arrival simulator
# 	- mkdir traces
# 	for util in $(UTILS); do \
# 		./simulator "FIFO" dists/$(basename $@).dist dists/$(basename $@)_$$util.arrival traces/$(basename $@)_$$util.trace ;\
# 	done

# %.plot: %.trace
# 	python3 Plot.py -t "traces/w2_.9.trace,traces/w2_.5.trace,traces/w2_.25.trace"

queuebisect: clean simulator distgen
	./queuebisect.sh

clean:
	rm -rf simulator
	rm -rf distgen
# rm -rf dists/*
	rm -rf traces/*
