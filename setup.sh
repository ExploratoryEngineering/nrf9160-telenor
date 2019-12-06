#!/bin/bash

# exit script if one of the commands fails
set -e 

# create a new virtual env
virtualenv --clear -p python3 venv
source venv/bin/activate

# install west in the venv
pip install west

rm -rf deps

# fetch dependencies defined in manifest/west.yaml
west update

# hack until nrfutil v6 is released
python scripts/fix-nrf-requirements.py

# install python dependencies
pip install --no-cache-dir -r deps/zephyr/scripts/requirements.txt
pip install --no-cache-dir -r deps/nrf/scripts/requirements.txt
pip install --no-cache-dir -r deps/mcuboot/scripts/requirements.txt
pip install --no-cache-dir -r scripts/requirements.txt
