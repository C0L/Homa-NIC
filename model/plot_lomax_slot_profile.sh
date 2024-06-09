#!/bin/bash

shopt -s extglob

python3 plot_lomax_slot_profile.py -t lomax_prof_traces/*.slotstats -f img/lomax_slot_profile.png
