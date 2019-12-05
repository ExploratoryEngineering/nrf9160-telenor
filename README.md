# Telenor nrf9160 samples

This repository includes a few sample applications to help get you started with
using the nRF9160 with the Telenor.

## Prerequesites

Nordic Semiconductors have chosen Zephyr for firmware development on the
nRF9160. The build tool for Zephyr relies on python, so to avoid potential
version conflicts of dependencies, we recommend installing the pip dependencies
locally in the project using a virtualenv.

* [nRF Connect for desktop](https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Connect-for-desktop) v3.3.0 (or newer)
    * Install and open the Getting started assistant
    * Follow all the steps in «Install the toolchain»

* Install virtualenv 16.7.0 (or newer)

    ```sh
        # check version
        virtualenv --version
    
        # install/upgrade virtualenv
        pip3 install virtualenv -U
    ```

* Set environment variables
    * Windows (make sure the path to gnuarmemb is correct)

        ```bat
            setx ZEPHYR_TOOLCHAIN_VARIANT gnuarmemb
            setx GNUARMEMB_TOOLCHAIN_PATH C:\gnuarmemb
        ````
    
    * MacOS

        ```sh
            echo "export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb" >> ~/.profile
            echo "export GNUARMEMB_TOOLCHAIN_PATH=/usr/local/opt/gcc-arm-none-eabi" >> ~/.profile
        ```
    
    * Linux

        ```sh
            # depends on your distro
            echo "export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb" >> ~/.profile
            echo "export GNUARMEMB_TOOLCHAIN_PATH=/usr" >> ~/.profile
        ```

* MacOS only
    * Install libgit2

        `brew install libgit2`


## Setup MacOS/Linux

1. `git clone ...` (insert this project's URL)
1. `./setup.sh`

## Build and run samples MacOS/Linux

1. `source venv/bin/activate` # activate the python virtualenv
1. `west build samples/hello_world` # build the hello world sample
1. `west flash` # flash the nrf9160 with the built binary


## Setup Windows PowerShell

1. `git clone ...` (insert this project's URL)
1. `.\setup.ps1`

## Build and run samples Windows PowerShell

1. `source .\venv\Scripts\activate` # activate the python virtualenv
1. `west build .\samples\hello_world` # build the hello world sample
1. `west flash` # flash the nrf9160 with the built binary


## Troubleshooting

### Delete the build folder

Very often when a build fails, some files are left in the `build/` folder which confuses subsequent builds. Try to delete the `build/` folder and build again.

### Clear Zephyr toolchain capability cache

Sometimes when changing dependency versions, the cache can cause build errors.
Deleting it doesn't do any damage, it just increases the build time on the next
build.

    rm -rf ~/Library/Caches/zephyr/ToolchainCapabilityDatabase
