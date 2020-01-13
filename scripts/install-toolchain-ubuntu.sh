#!/bin/bash

# wget https://github.com/ExploratoryEngineering/nrf9160-telenor/archive/master.zip
# ...


# update package index
sudo apt update

# install apt dependencies
sudo apt-get -y install git wget file ninja-build gperf ccache dfu-util make gdb-multiarch xz-utils python3.7 python3-pip

# install pipenv
python3 -m pip install --user pipenv
PATH="$HOME/.local/bin:$PATH"

# install device-tree-compiler
wget http://mirrors.kernel.org/ubuntu/pool/main/d/device-tree-compiler/device-tree-compiler_1.4.7-1_amd64.deb
sudo dpkg -i device-tree-compiler_1.4.7-1_amd64.deb

# install cmake
sudo wget -P /opt -nv https://github.com/Kitware/CMake/releases/download/v3.16.2/cmake-3.16.2-Linux-x86_64.tar.gz
sudo tar zxf /opt/cmake-3.16.2-Linux-x86_64.tar.gz -C /opt
sudo ln -s /opt/cmake-3.16.2-Linux-x86_64/bin/* /usr/local/sbin

# gcc-arm-none-eabi
sudo wget -P /opt -nv https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2019q3/RC1.1/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2
sudo tar jxf /opt/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2 -C /opt

# set environment variables
echo "export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb" >> ~/.profile
echo "export GNUARMEMB_TOOLCHAIN_PATH=/opt/gcc-arm-none-eabi-8-2019-q3-update" >> ~/.profile

source ~/.profile
