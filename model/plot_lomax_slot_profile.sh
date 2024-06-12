#!/bin/bash

shopt -s extglob

python3 plot_slot_occupancy.py -t lomax_prof_traces/*.slotstats -f img/lomax_slot_occupancy.png
python3 plot_slot_min_backlog.py -t lomax_prof_traces/*.slotstats -f img/lomax_slot_min_backlog.png
python3 plot_slot_min_total_backlog.py -t lomax_prof_traces/*.slotstats -f img/lomax_slot_min_total_backlog.png
python3 plot_slot_backlog.py -t lomax_prof_traces/*.slotstats -f img/lomax_slot_backlog.png
python3 plot_slot_total_backlog.py -t lomax_prof_traces/*.slotstats -f img/lomax_slot_total_backlog.png

