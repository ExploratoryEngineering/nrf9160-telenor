# Hello world

This example prints the device IMSI and IMEI, connects to the mda.ee APN, and sends a simple message.

## Prerequisites

Please make sure you've followed [the instructions in the main README](../../README.md) before proceeding.

## Clean build folder

If you've already built another sample, you'll get build errors because the `build/` folder contains files from a different source directory. So we need to clean the `build/` folder before building a different sample.

```sh
# macOS/Linux clean old build folder
rm -rf build # clean the build folder (in case you built hello_world first)

# Windows clean old build folder
rd /s /q build
```

Alternately, you can build each sample in its own directory; then their build folders will not collide.

## Build and run

Now, let's get down to the business of sending data over NB-IoT.  Once you have your serial terminal application open and connected, and you have connected the nRF9160 DK to your computer via USB, build and flash the example application to the nRF9160 DK, as follows:

```sh
pipenv run west build samples/hello_world # build the hello world sample
pipenv run west flash # flash the nrf9160 with the built binary
```

_Note: The default board is the nRF9160 Development Kit. If you want to build and upload to another nrf9160-based board, you have to add `-b <board-name>` for the build command above. So to build for the Thingy:91, the command would be: `pipenv run  west build -b nrf9160_pca20035ns samples/hello_world`_

Once the application is flashed to the device, it will immediately begin running. In your serial terminal application you will see a lot of output about Zephyr booting, then possibly some delay, and then you will see the following application output:

	Example application started.
	IMEI: <imei>
	IMSI: <imsi>
	Connecting...

At this point, the application will try to connect to the IoT network, but it will not succeed because the device is not yet registered on the NB-IoT Developer Platform. Now is the time to copy the IMEI and IMSI and register the device as described in the [Getting Started](https://docs.nbiot.engineering/tutorials/getting-started.html) tutorial.

After registering the device, restart the application by pressing the RESET button on the nRF9160 DK. You will see the same output as before, but after some time (15-20 seconds, be patient!) you will additionally see that the device connected to the network and sent a message. On the device page on the Developer Platform you will also see that the message has been successfully transmitted.

Well done!
