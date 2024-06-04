#!/bin/bash

shopt -s extglob

python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w1_1.05.png -w w1 -u 1.05 &
python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w1_1.1.png -w w1 -u 1.1 & 
python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w1_1.2.png -w w1 -u 1.2 &
python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w1_1.3.png -w w1 -u 1.3 &

python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w2_1.05.png -w w2 -u 1.05 &
python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w2_1.1.png -w w2 -u 1.1 &
python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w2_1.2.png -w w2 -u 1.2 &
python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w2_1.3.png -w w2 -u 1.3 &

python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w3_1.05.png -w w3 -u 1.05 &
python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w3_1.1.png -w w3 -u 1.1 &
python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w3_1.2.png -w w3 -u 1.2 &
python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w3_1.3.png -w w3 -u 1.3 &

python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w4_1.05.png -w w4 -u 1.05 &
python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w4_1.1.png -w w4 -u 1.1 &
python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w4_1.2.png -w w4 -u 1.2 &
python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w4_1.3.png -w w4 -u 1.3 &
# 
# wait

# python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w1_w2.png
# python3 PlotBurststats.py -t traces/w1*burst*.slotstats -f img/slotstats_burst.png
# python3 PlotSlotstats.py -t traces/*@(1.2|1.3|1.4|1.5|1.6)*-1*-1*.slotstats -f img/slotstats.png

#python3 PlotSimstats.py -t traces/*@(.1|.5|.9|.99|.999|.9999)* -f img/simstats.png
# python3 PlotSlotstats.py -t traces/w1_*@(.9|.99|.999)*-1*-1*.slotstats -f img/slotstats.png
