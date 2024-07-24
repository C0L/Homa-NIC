#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )

TRACEDIR=pifo_insertion_pulse

for wk in "${WORKLOADS[@]}"; do
    python3 plot_drain_inaccuracy.py -t ${TRACEDIR}/${wk}_* -f img/pifo_insertion_pulse_inaccuracy_${wk}.png
    python3 plot_drain_true_DOE.py -t ${TRACEDIR}/${wk}_* -f img/pifo_insertion_pulse_true_DOE_${wk}.png
    python3 plot_drain_DOE.py -t ${TRACEDIR}/${wk}_* -f img/pifo_insertion_pulse_DOE_${wk}.png
    python3 plot_drain_presorted.py -t ${TRACEDIR}/${wk}_* -f img/pifo_insertion_pulse_presorted_${wk}.png
    python3 plot_drain_p99_violations.py -t ${TRACEDIR}/${wk}_* -f img/pifo_insertion_pulse_p99_violations_${wk}.png
    python3 plot_drain_lowwater.py -t ${TRACEDIR}/${wk}_* -f img/pifo_insertion_pulse_lowwater_${wk}.png
done
