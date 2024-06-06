#!/bin/bash

shopt -s extglob

python3 PlotPifoLogn.py -t paretoefficient_lomax_pifo_logn_traces/ -f img/pareto_efficient_lomax_pifo_logn_w1.png -w w1 -u 1.05
# python3 PlotBurststats.py -t paretoefficient_lomax_pifo_logn_traces/ -f img/pareto_efficient_lomax_pifo_logn_w1.png -w w1 -u 1.05
# python3 PlotBurststats.py -t lomaxpareto-traces/ -f img/pareto_efficient_w1.png -w w1 -u 1.05 1.1 1.2 1.3 &
# python3 PlotBurststats.py -t lomaxpareto-traces/ -f img/pareto_efficient_w2.png -w w2 -u 1.05 1.1 1.2 1.3 &
# python3 PlotBurststats.py -t lomaxpareto-traces/ -f img/pareto_efficient_w3.png -w w3 -u 1.05 1.1 1.2 1.3 &
# python3 PlotBurststats.py -t lomaxpareto-traces/ -f img/pareto_efficient_w4.png -w w4 -u 1.05 1.1 1.2 1.3 &
# python3 PlotBurststats.py -t lomaxpareto-traces/ -f img/pareto_efficient_w5.png -w w1 -u 1.05 1.1 1.2 1.3
# python3 PlotBurststats.py -t traces.bak/ -f img/pareto_efficient_w1_1.1.png -w w1 -u 1.1 & 
# python3 PlotBurststats.py -t traces.bak/ -f img/pareto_efficient_w1_1.2.png -w w1 -u 1.2 &
# python3 PlotBurststats.py -t traces.bak/ -f img/pareto_efficient_w1_1.3.png -w w1 -u 1.3 &
# 
# python3 PlotBurststats.py -t traces.bak/ -f img/pareto_efficient_w2_1.05.png -w w2 -u 1.05 &
# python3 PlotBurststats.py -t traces.bak/ -f img/pareto_efficient_w2_1.1.png -w w2 -u 1.1 &
# python3 PlotBurststats.py -t traces.bak/ -f img/pareto_efficient_w2_1.2.png -w w2 -u 1.2 &
# python3 PlotBurststats.py -t traces.bak/ -f img/pareto_efficient_w2_1.3.png -w w2 -u 1.3 &
# 
# python3 PlotBurststats.py -t traces.bak/ -f img/pareto_efficient_w3_1.05.png -w w3 -u 1.05 &
# python3 PlotBurststats.py -t traces.bak/ -f img/pareto_efficient_w3_1.1.png -w w3 -u 1.1 &
# python3 PlotBurststats.py -t traces.bak/ -f img/pareto_efficient_w3_1.2.png -w w3 -u 1.2 &
# python3 PlotBurststats.py -t traces.bak/ -f img/pareto_efficient_w3_1.3.png -w w3 -u 1.3 &
# 
# python3 PlotBurststats.py -t traces.bak/ -f img/pareto_efficient_w4_1.05.png -w w4 -u 1.05 &
# python3 PlotBurststats.py -t traces.bak/ -f img/pareto_efficient_w4_1.1.png -w w4 -u 1.1 &
# python3 PlotBurststats.py -t traces.bak/ -f img/pareto_efficient_w4_1.2.png -w w4 -u 1.2 &
# python3 PlotBurststats.py -t traces.bak/ -f img/pareto_efficient_w4_1.3.png -w w4 -u 1.3 &
