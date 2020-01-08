# Telenor nrf9160 samples

This repository includes a few sample applications to help get you started with
using the nRF9160 with the Telenor.

![](https://github.com/ExploratoryEngineering/nrf9160-telenor/workflows/Build%20Samples/badge.svg)

## Prerequesites

Nordic Semiconductors have chosen Zephyr for firmware development on the
nRF9160. The build tool for Zephyr relies on python. To produce a determnistic
build, we've created a Pipfile for [pipenv][1] with all the pip dependencies
from the [different][2] [Zephyr][3] [repositories][4]. Currently it's based on
[nRF Connect SDK][5] v1.1.0.

* [nRF Connect for desktop](https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Connect-for-desktop) v3.3.0 (or newer)
    * Install and open the Getting started assistant
    * Follow all the steps in «Install the toolchain»

* [nRF Command Line Tools](https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Command-Line-Tools) v10.5.0 (or newer)

* Install pipenv

    ```sh
        pip3 install pipenv
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


## Setup MacOS/Linux/Windows

1. `git clone https://github.com/ExploratoryEngineering/nrf9160-telenor`
1. `pipenv install` # install python dependencies in projects virtualenv
1. `pipenv run west update` # download the Zephyr dependencies using west

## Build and run samples MacOS/Linux

1. `pipenv shell` # activate the projects virtualenv
1. `west build samples/hello_world` # build the hello world sample
1. `west flash` # flash the nrf9160 with the built binary

_Note: The default board is the nRF9160 Development Kit. If you want to build and upload to another nrf9160-based board, you have to add `-b <board-name>` for the build command above. So to build for the Thingy:91, the command would be: `west build -b nrf9160_pca20035ns samples/hello_world`_

## Build and run samples Windows PowerShell

1. `pipenv shell` # activate the projects virtualenv
1. `west build .\samples\hello_world` # build the hello world sample
1. `west flash` # flash the nrf9160 with the built binary


## Build and debug with Visual Studio Code

TL;DR - you need to run `build <sample name>` before you run the `flash` task. Delete the `build` folder or run `pristine` before building another sample. See why further down.

### Build

1. The default build target is the nRF9160 Development Kit (nrf9160_pca10090ns)

    * If you want to build and upload to another nrf9160-based board, you have to replace `nrf9160_pca10090ns` with the board you'd like to build for in `.vscode/tasks.json`

1. Open the [Command Palette](https://code.visualstudio.com/docs/getstarted/tips-and-tricks#_command-palette) with <key>Ctrl ⇧ P</key> (Win/Linux) or <key>⇧ ⌘ P</key> (Mac)
1. Start typing «`run task`»
1. Select «`Tasks: Run Task`» using arrow keys and hit <key>↵</key>
1. Choose the sample you want to build using arrow keys and hit <key>↵</key>

![Task list screenshot](img/build.gif)

### Flash to nrf9160

1. First build using the steps above
1. Follow the same steps as above to open the task list
1. Choose `flash` to upload the binary from the build step to the connected nrf9160 DK

![Task list screenshot](img/flash.gif)

### Debug

1. First build and flash one of the samples
1. Set a breakpoint in the source code where you want it to break
1. Press <key>F5</key> - this should open a debugging session (or use the debug tab)
1. The debugger will first stop in `reset.S` before the application is loaded
1. To get to the application code, press <key>F5</key> or the play button to continue executing code
1. The debugger should stop at your breakpoint, and you can step in/out/over, see variables (see note below) and call stack

![Task list screenshot](img/debug.gif)

_Note: The compiler will try to optimize your code, so you'll probably see `<optimized out>` under variables instead of the variable value. To disable the optimizations when debugging, add these lines to the prj.conf:_

```ini
    CONFIG_NO_OPTIMIZATIONS=y
    CONFIG_DEBUG=y
```

We've included configuration to make it possible to build the samples and debug on the nrf9160 from [Visual Studio Code](https://code.visualstudio.com/).

First, let me explain an important feature in West (Zephyrs build tool). When you build an application/sample using west, the output will be in the `build` folder of the current working directory.

Say you've cloned this project into `~/nrf9160-telenor`:
* If you run `west build samples/hello_world` from `~/nrf9160-telenor`, the output will be in `~/nrf9160-telenor/build`. It will also store what path you used when building. So if you now just run `west build` without specifying a path, it will still build `hello_world`. If you want to switch sample, you have to run `west build -t pristine` or delete the `build` folder.
* If you `cd` into the `samples/hello_world` folder and run `west build` there, it will output the build folder to `samples/hello_world/build`.

Both methods work fine, but the [tasks](https://code.visualstudio.com/docs/editor/tasks) we've defined in VS Code always build from the project root folder. Then we only need to duplicate the `build ...` task for each sample, and the `flash` and `debug` assume you've already ran the build task first.

## Tips'n tricks

### Menuconfig

The amount of config options for Zephyr can be quite daunting, but they actually have a command line user interface that allows you to browse the config options interactiely. Either navigate using arrow keys or search using the <key>`/`</key> key. When you find the right options, write down the name and value. Alternatively save a minimal config with the <key>`D`</key> key to a temporary file, then copy the options over to `prj.conf`.

```sh
    west build -t menuconfig samples/hello_world
```

## Troubleshooting

### Delete the build folder

Very often when a build fails, some files are left in the `build/` folder which confuses subsequent builds. Try to delete the `build/` folder and build again.

### Clear Zephyr toolchain capability cache

Sometimes when changing dependency versions, the cache can cause build errors.
Deleting it doesn't do any damage, it just increases the build time on the next
build.

    rm -rf ~/Library/Caches/zephyr/ToolchainCapabilityDatabase

[1]: https://pipenv-fork.readthedocs.io/en/latest/
[2]: https://github.com/NordicPlayground/fw-nrfconnect-nrf/blob/master/scripts/requirements.txt
[3]: https://github.com/NordicPlayground/fw-nrfconnect-zephyr/blob/master/scripts/requirements.txt
[4]: https://github.com/NordicPlayground/fw-nrfconnect-mcuboot/blob/master/scripts/requirements.txt
[5]: https://github.com/NordicPlayground/fw-nrfconnect-zephyr
