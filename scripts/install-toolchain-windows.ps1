#!/usr/bin/env pwsh

$ErrorActionPreference = 'stop'

try {
    if(Get-Command 'choco'){
        echo 'choco already installed'
    }
} Catch {
    echo 'choco not installed'
    # install choco
    iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
}

choco install -y cmake --installargs 'ADD_CMAKE_TO_PATH=System' --no-progress
choco install git 7zip ninja dtc-msys2 gperf --no-progress
$ProgressPreference = 'SilentlyContinue'
Invoke-WebRequest https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2019q3/RC1.1/gcc-arm-none-eabi-8-2019-q3-update-win32.zip -OutFile gcc-arm-win32.zip
7z x -bd -oC:\gnuarmemb .\gcc-arm-win32.zip
