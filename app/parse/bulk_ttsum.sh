#!/bin/bash

for f in perf_write_*; do python3 ttsum.py -f "write request" $f; done
