#!/bin/bash

#Install python dependency 
pip install --upgrade pip

#Install pico_zense_cython_wrapper
cd pico_zense_cython_wrapper
pip install -r requirements.txt  --user
python setup.py install --user
cd ..