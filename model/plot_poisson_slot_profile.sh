#!/bin/bash

shopt -s extglob

python3 plot_slot_occupancy.py -t poisson_prof_traces/*.slotstats -f img/poisson_slot_occupancy.png
python3 plot_slot_min_backlog.py -t poisson_prof_traces/*.slotstats -f img/poisson_slot_min_backlog.png
python3 plot_slot_min_total_backlog.py -t poisson_prof_traces/*.slotstats -f img/poisson_slot_min_total_backlog.png
python3 plot_slot_backlog.py -t poisson_prof_traces/*.slotstats -f img/poisson_slot_backlog.png
python3 plot_slot_total_backlog.py -t poisson_prof_traces/*.slotstats -f img/poisson_slot_total_backlog.png
