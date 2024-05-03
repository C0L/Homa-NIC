.PHONY: distgen
distgen:
	g++ -c dist.cc
	g++ -Idist.hh distgen.cc dist.o -o distgen

.PHONY: dists
dists: distgen
	- mkdir dists
	./distgen 'w1' 1000000 > dists/w1
	./distgen 'w2' 1000000 > dists/w2
	./distgen 'w3' 1000000 > dists/w3
	./distgen 'w4' 1000000 > dists/w4
	./distgen 'w5' 1000000 > dists/w5

clean:
	rm -rf distgen
