#!/bin/bash

shopt -s extglob

# python3 plot_slot_occupancy.py -t poisson_prof_traces/*.slotstats -f img/poisson_slot_occupancy.png
# python3 plot_slot_min_backlog.py -t poisson_prof_traces/*.slotstats -f img/poisson_slot_min_backlog.png
# python3 plot_slot_min_total_backlog.py -t poisson_prof_traces/*.slotstats -f img/poisson_slot_min_total_backlog.png
# python3 plot_slot_backlog.py -t poisson_prof_traces/*.slotstats -f img/poisson_slot_backlog.png

python3 plot_slot_tts.py -t poisson_prof_traces/w1*.9999*.slotstats -f img/poisson_slot_tts_w1.png
python3 plot_slot_tts.py -t poisson_prof_traces/w2*.9999*.slotstats -f img/poisson_slot_tts_w2.png
python3 plot_slot_tts.py -t poisson_prof_traces/w3*.9999*.slotstats -f img/poisson_slot_tts_w3.png
# python3 plot_slot_tts.py -t poisson_prof_traces/w4*.9999*.slotstats -f img/poisson_slot_tts_w4.png
# python3 plot_slot_tts.py -t poisson_prof_traces/w5*.9999*.slotstats -f img/poisson_slot_tts_w5.png
python3 plot_slot_occupancy.py -t poisson_prof_traces/w1*.9999*.slotstats -f img/poisson_slot_occupancy_w1.png
python3 plot_slot_occupancy.py -t poisson_prof_traces/w2*.9999*.slotstats -f img/poisson_slot_occupancy_w2.png
python3 plot_slot_occupancy.py -t poisson_prof_traces/w3*.9999*.slotstats -f img/poisson_slot_occupancy_w3.png
# python3 plot_slot_occupancy.py -t poisson_prof_traces/w4*.9999*.slotstats -f img/poisson_slot_occupancy_w4.png
# python3 plot_slot_occupancy.py -t poisson_prof_traces/w5*.9999*.slotstats -f img/poisson_slot_occupancy_w5.png
