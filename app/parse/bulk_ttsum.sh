#!/bin/bash

rm parsed
for f in perf_write_*; do echo $f; python3 ttsum.py -a -f "write request" $f; done >> parsed
