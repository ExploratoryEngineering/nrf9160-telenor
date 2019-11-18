#!/bin/bash

set -e 

cd "$(dirname "$0")"

# cleanup
deactivate || echo "Virtualenv not active"
rm -rf venv
git checkout -- nrf/scripts/requirements.txt
# rm -rf ~/Library/Caches/zephyr/ToolchainCapabilityDatabase

# create a new virtual env
virtualenv venv
source venv/bin/activate

# install west in the venv
pip3 install west

# fetch dependencies defined in manifest/west.yaml
west update

# hack until nrfutil v6 is released
sed -i -e "s/^nrfutil$/nrfutil>=6.0.0a1/" nrf/scripts/requirements.txt

# install python dependencies
pip3 install --no-cache-dir -r zephyr/scripts/requirements.txt
pip3 install --no-cache-dir -r nrf/scripts/requirements.txt
pip3 install --no-cache-dir -r mcuboot/scripts/requirements.txt
