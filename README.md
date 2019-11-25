# Telenor nrf9160 samples

This repository includes a few sample applications to help get you started with
using the nRF9160 with the Telenor.

## Prerequesites

Nordic Semiconductors have chosen Zephyr for firmware development on the
nRF9160. The build tool for Zephyr relies on python, so to avoid potential
version conflicts of dependencies, we recommend installing the pip dependencies
locally in the project using a virtualenv.

* [nRF Connect for desktop](https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Connect-for-desktop) v3.3.0 (or newer)
    * Open the Getting started assistant
    * Follow all the steps in «Install the toolchain»

* Install virtualenv 16.7.0 (or newer)

```sh
    # check version
    virtualenv --version
    
    # install/upgrade virtualenv
    pip3 install virtualenv -U
```

* MacOS only
    * Install libgit2

        `brew install libgit2`


## Setup

1. `git clone ...` (insert this project's URL)
1. `./setup.sh`

## Build and run samples

1. `source venv/bin/activate`
1. `west build samples/hello_world`
1. `west flash`
