#!/bin/bash

for i in {0..64}; do
    python3 plot_slot_total_backlog_single.py -t prof_traces_non_monotonic/*_${i}.slotstats -f img/non_monotonic_total_backlog_${i}.png
done
