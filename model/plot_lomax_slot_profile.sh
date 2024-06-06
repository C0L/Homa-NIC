#!/bin/bash

shopt -s extglob

# python3 PlotBurststats.py -t traces/ -f img/pareto_efficient_w1_w2.png
# python3 PlotBurststats.py -t traces/w1*burst*.slotstats -f img/slotstats_burst.png
# python3 PlotSlotstats.py -t traces/*@(1.2|1.3|1.4|1.5|1.6)*-1*-1*.slotstats -f img/slotstats.png

# python3 PlotSimstats.py -t traces/*@(.1|.5|.9|.99|.999|.9999)* -f img/simstats.png
python3 PlotSlotstats.py -t lomaxprof-traces/*.slotstats -f img/lomaxprof.png
