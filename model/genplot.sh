#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )

for wk in "${WORKLOADS[@]}";
do
    # python3 Plot.py -t traces/${wk}_*.rate -f mm1_${wk}_rate.png -x "Slot Index" -y "Rate" -T "Rate by Slot Index"
    # python3 Plot.py -t traces/${wk}_*.mass -f mm1_${wk}_mass.png -x "Slot Index" -y "Mass" -T "Mass by Slot Index"
    python3 PlotMCTs.py -t traces/${wk}_*.simstats -f img/mm1_${wk}_mcts.png
done
