#!/bin/bash

python3 plot_fct_slowdown_inacc.py -t hw_model_poisson/w1*.slotstats -f img/hw_model_poisson_w1.png
python3 plot_fct_slowdown_inacc.py -t hw_model_poisson/w2*.slotstats -f img/hw_model_poisson_w2.png
python3 plot_fct_slowdown_inacc.py -t hw_model_poisson/w3*.slotstats -f img/hw_model_poisson_w3.png
python3 plot_fct_slowdown_inacc.py -t hw_model_poisson/w4*.slotstats -f img/hw_model_poisson_w4.png
python3 plot_fct_slowdown_inacc.py -t hw_model_poisson/w5*.slotstats -f img/hw_model_poisson_w5.png
