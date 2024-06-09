#!/bin/bash

shopt -s extglob

python3 plot_pareto_efficient_lomax_pifo_logn.py -t pareto_efficient_lomax_pifo_logn_traces/ -f img/pareto_efficient_lomax_pifo_logn_w1.png -w w1 -u 1.115
python3 plot_pareto_efficient_lomax_pifo_logn.py -t pareto_efficient_lomax_pifo_logn_traces/ -f img/pareto_efficient_lomax_pifo_logn_w2.png -w w2 -u 1.115
python3 plot_pareto_efficient_lomax_pifo_logn.py -t pareto_efficient_lomax_pifo_logn_traces/ -f img/pareto_efficient_lomax_pifo_logn_w3.png -w w3 -u 1.115
python3 plot_pareto_efficient_lomax_pifo_logn.py -t pareto_efficient_lomax_pifo_logn_traces/ -f img/pareto_efficient_lomax_pifo_logn_w4.png -w w4 -u 1.115
python3 plot_pareto_efficient_lomax_pifo_logn.py -t pareto_efficient_lomax_pifo_logn_traces/ -f img/pareto_efficient_lomax_pifo_logn_w5.png -w w5 -u 1.115
