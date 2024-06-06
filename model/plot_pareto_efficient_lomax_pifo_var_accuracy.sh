#!/bin/bash

shopt -s extglob

python3 PlotBurststats.py -t lomaxpareto-traces/ -f img/pareto_efficient_w1.png -w w1 -u 1.05 1.1 1.2 1.3 &
python3 PlotBurststats.py -t lomaxpareto-traces/ -f img/pareto_efficient_w1.png -w w1 -u 1.05 1.1 1.2 1.3 &
python3 PlotBurststats.py -t lomaxpareto-traces/ -f img/pareto_efficient_w2.png -w w2 -u 1.05 1.1 1.2 1.3 &
python3 PlotBurststats.py -t lomaxpareto-traces/ -f img/pareto_efficient_w3.png -w w3 -u 1.05 1.1 1.2 1.3 &
