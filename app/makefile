CC=g++
CFLAGS=-mavx2 -mavx512f -std=c++17

SRCS := $(wildcard src/*.cc)
HDRS := $(wildcard src/*.h)
OBJS := $(patsubst src/%.cc,bin/%.o,$(SRCS))

BINS=bin/perf_write

.PHONY: parse
parse:
	@echo "64 Byte Echo Perf"
	python3 ./parse/ttsum.py -f "write request" perf_write.tt
	@echo "64 Byte Device Register Read Perf"
	python3 ./parse/ttsum.py -f "read request" perf_read.tt

%.tt: $(BINS)
	sudo ./$<

$(BINS): $(OBJS) $(HDRS) 
	@mkdir -p bin
	$(CC) $(CFLAGS) $(OBJS) -o $@

bin/%.o: src/%.cc $(HDRS)
	@mkdir -p bin
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(BINS) $(OBJS)
