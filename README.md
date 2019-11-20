# Telenor nrf9160 samples

This repository includes a few sample applications to help get you started with using the nRF9160 with the Telenor.

## Setup

1. Install dependencies as explained in [nRF9160 DK getting started](https://docs.nbiot.engineering/tutorials/nrf9160-basic.html)
1. `git clone ...` (insert this project's URL)
1. `brew install libgit2`
1. `./setup.sh`

## Build and run samples

1. `cd samples/hello_world`
1. `west build -b nrf9160_pca10090ns`
1. `west flash`
