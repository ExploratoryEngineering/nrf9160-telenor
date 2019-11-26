#!/bin/bash

set -e 

# cleanup
rm -rf venv

if [ -d "nrf" ]; then
    cd deps/nrf
    git checkout -- deps/nrf/scripts/requirements.txt
    cd ../..
fi

# rm -rf ~/Library/Caches/zephyr/ToolchainCapabilityDatabase

# create a new virtual env
virtualenv -p python3 venv
source venv/bin/activate

# install west in the venv
pip3 install west

# fetch dependencies defined in manifest/west.yaml
west update

# hack until nrfutil v6 is released
sed -i -e "s/^nrfutil$/nrfutil>=6.0.0a1/" deps/nrf/scripts/requirements.txt

# install python dependencies
pip3 install --no-cache-dir -r deps/zephyr/scripts/requirements.txt
pip3 install --no-cache-dir -r deps/nrf/scripts/requirements.txt
pip3 install --no-cache-dir -r deps/mcuboot/scripts/requirements.txt
