CC=g++
CFLAGS=-mavx2 -mavx512f -std=c++17

SRCS := $(wildcard src/*.cc)

LIBSC := lib/*.cc
LIBSH := $(wildcard lib/*.h)
LIBSO := $(patsubst lib/%.cc,bin/%.o,$(LIBSC))

# PERF_WRITEC := src/perf_write.cc
# PERF_WRITEH := $(wildcard src/*.h)
# PERF_WRITEO := $(patsubst src/%.cc,bin/%.o,$(PERF_WRITEC))
# 
# PERF_READC := src/perf_read.cc
# PERF_READH := $(wildcard src/*.h)
# PERF_READO := $(patsubst src/%.cc,bin/%.o,$(PERF_READC))
# 
# PERF_MSGC := src/perf_msg.cc
# PERF_MSGH := $(wildcard src/*.h)
# PERF_MSGO := $(patsubst src/%.cc,bin/%.o,$(PERF_MSGC))

.PHONY: parse
parse: perf_write.tt perf_read.tt
	@echo "64 Byte Echo Perf"
	python3 ./parse/ttsum.py -f "write request" ./parse/perf_write.tt
	@echo "64 Byte Device Register Read Perf"
	python3 ./parse/ttsum.py -f "read request" ./parse/perf_read.tt

%.tt: bin/%
	sudo ./$< 


# bin/perf_msg: $(PERF_MSGO) $(PERF_MSGH) $(LIBSO)
# 	@mkdir -p bin
# 	$(CC) $(CFLAGS) $(PERF_MSGO) $(LIBSO) -o $@
# 
# bin/perf_write: $(PERF_WRITEO) $(PERF_WRITEH) $(LIBSO)
# 	@mkdir -p bin
# 	$(CC) $(CFLAGS) $(PERF_WRITEO) $(LIBSO) -o $@
# 
# bin/perf_read: $(PERF_READO) $(PERF_READH) $(LIBSO)
# 	@mkdir -p bin
# 	$(CC) $(CFLAGS) $(PERF_READO) $(LIBSO) -o $@

bin/%: bin/%.o $(LIBSO)
	@mkdir -p bin
	$(CC) $(CFLAGS) %.o $(LIBSO) -o $@

bin/%.o: src/%.cc $(HDRS)
	@mkdir -p bin
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f bin/* parse/*.tt 
