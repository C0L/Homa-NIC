#!/bin/bash

python3 PlotStats.py -t traces/*.simstats -f img/watermarks.png
# python3 PlotMCTs.py -t traces/*_0_0*.simstats -f img/no_comm_comb_mcts.png
# python3 PlotMCTs.py -t traces/*_140_40*.simstats -f img/comm_nsort_comb_mcts.png
# python3 PlotMCTs.py -t traces/*_140_0*.simstats -f img/comm_sort_comb_mcts.png
# python3 PlotMass.py -t traces/*-1_-1_0_0_0*.mass -f img/mm1_comb_mass.png
# python3 PlotRate.py -t traces/*-1_-1_0_0_0*.rate -f img/mm1_comb_rates.png
