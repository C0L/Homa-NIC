#!/bin/bash

shopt -s extglob

python3 plot_pareto_efficient_lomax_pifo_lin.py -t pareto_efficient_lomax_pifo_lin_traces/ -f img/pareto_efficient_lomax_pifo_lin_w1.png -w w1 -u 1.115
python3 plot_pareto_efficient_lomax_pifo_lin.py -t pareto_efficient_lomax_pifo_lin_traces/ -f img/pareto_efficient_lomax_pifo_lin_w2.png -w w2 -u 1.115
python3 plot_pareto_efficient_lomax_pifo_lin.py -t pareto_efficient_lomax_pifo_lin_traces/ -f img/pareto_efficient_lomax_pifo_lin_w3.png -w w3 -u 1.115
python3 plot_pareto_efficient_lomax_pifo_lin.py -t pareto_efficient_lomax_pifo_lin_traces/ -f img/pareto_efficient_lomax_pifo_lin_w4.png -w w4 -u 1.115
python3 plot_pareto_efficient_lomax_pifo_lin.py -t pareto_efficient_lomax_pifo_lin_traces/ -f img/pareto_efficient_lomax_pifo_lin_w5.png -w w5 -u 1.115
