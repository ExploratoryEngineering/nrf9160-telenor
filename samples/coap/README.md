# Constrained Application Protocol (CoAP)

This sample demonstrates how a device can act as a CoAP node (client and server).

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

Navigate to the Live Stream tab of the device you've registered in the [Telenor NB-IoT Service](https://nbiot.engineering/).  Then build the firmware and flash it to the device by running

```sh
pipenv west build
pipenv west flash
```

After the device boots, it may take some time before it is connected to the network.  Then, it will send a CoAP POST message upstream and start listening for downstream messages.  You should see the upstream message in the Live Stream tab.

To send a downstream message, select `CoAP` in the Message Type field, enter `message` in the Path field, enter a payload string, check the Encode Payload box, and click Send Message.  The device should report having received the POST request in its serial output.

See [the documentation](https://docs.nbiot.engineering/) for how to send and receive CoAP messages via the REST API and client libraries.
