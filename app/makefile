CC=g++
CFLAGS=-mavx2 -mavx512f -std=c++17 -Ilib

LIBSC := $(wildcard lib/*.cc)
LIBSH := $(wildcard lib/*.h)
LIBSO := $(patsubst lib/%.cc,lib/%.o,$(LIBSC))

# PERFS : = perf_write perf_read perf_msg
APPS := bin/perf_write \
	bin/perf_read \
	bin/perf_msg \
	bin/perf_latency_server \
	bin/perf_latency_client \
	bin/perf_bitrate_server \
	bin/perf_bitrate_client \
	bin/perf_msgrate_server \
	bin/perf_msgrate_client


.PHONY: parse
.PHONY: clean
# parse: $(patsubst %,%.tt,$(PERFS))
	# @echo "64 Byte Echo Perf"
	# python3 ./parse/ttsum.py -f "write request" ./parse/perf_write.tt
	# @echo "64 Byte Device Register Read Perf"
	# python3 ./parse/ttsum.py -f "read request" ./parse/perf_read.tt
	# @echo "64 Byte Loopback"

# python3 ./parse/ttsum.py -f "write request" ./parse/perf_msg.tt

%.tt: bin/%
	sudo ./$<

apps: $(APPS)

bin/%: src/%.o $(LIBSO)
	$(CC) $(CFLAGS) $< $(LIBSO) -o $@

%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f lib/*.o src/*.o parse/*.tt bin/*
