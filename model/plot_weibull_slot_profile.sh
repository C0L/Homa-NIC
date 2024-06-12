#!/bin/bash

shopt -s extglob

python3 plot_slot_occupancy.py -t weibull_prof_traces/*.slotstats -f img/weibull_slot_occupancy.png
python3 plot_slot_min_backlog.py -t weibull_prof_traces/*.slotstats -f img/weibull_slot_min_backlog.png
python3 plot_slot_min_total_backlog.py -t weibull_prof_traces/*.slotstats -f img/weibull_slot_min_total_backlog.png
python3 plot_slot_backlog.py -t weibull_prof_traces/*.slotstats -f img/weibull_slot_backlog.png
python3 plot_slot_total_backlog.py -t weibull_prof_traces/*.slotstats -f img/weibull_slot_total_backlog.png

