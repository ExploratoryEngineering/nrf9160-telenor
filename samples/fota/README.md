# Firmware over the air (FOTA)

Before deploying a device into the field, you probably want to have a way to update it without someone having to manually get to the device and install a new firmware change. The [Telenor IoT Gateway](https://nbiot.engineering/) has implemented the firmware update part of LwM2M standard to simplify the process of firmware updates. This sample implements the minimum of what you need on the device side to do a firwmare update over the air. Full details of [how to do a firmware update is documented on our blog](https://blog.exploratory.engineering/post/something-in-the-air/).

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

## Build and run fota

Build the firmware and flash it to the device by running

```sh
pipenv run west build
pipenv run west flash
```

_Note: The default board is the nRF9160 Development Kit. If you want to build and upload to another nrf9160-based board, you have to add `-b <board-name>` for the build command above. So to build for the Thingy:91, the command would be: `west build -b nrf9160_pca20035ns samples/fota`_

Now, modify the firmware version on the command line (or by editing [prf.conf](prj.conf)), build again (but don't flash!), upload the image to the Telenor IoT Gateway, and mark the device for upgrade, as follows:

```sh
rm -rf build  # unfortunately necessary, or Zephyr will not see the version change
pipenv run west build -- -DCONFIG_APP_FIRMWARE_VERSION=\"1.0.1\"
curl -HX-API-Token:<token> -F image=@build/zephyr/app_update.bin                           https://api.nbiot.telenor.io/collections/<collection-id>/firmware
curl -HX-API-Token:<token> -XPATCH -d'{"version": "1.0.1"}'                                https://api.nbiot.telenor.io/collections/<collection-id>/firmware/<firmware-id>
curl -HX-API-Token:<token> -XPATCH -d'{"firmware": {"management": "device"}}'              https://api.nbiot.telenor.io/collections/<collection-id>
curl -HX-API-Token:<token> -XPATCH -d'{"firmware": {"targetFirmwareId": "<firmware-id>"}}' https://api.nbiot.telenor.io/collections/<collection-id>/devices/<device-id>
```

with `<token>`, `<collection-id>`, `<device-id>`, and `<firmware-id>` replaced by appropriate values.  The output of the first `curl` command will give you the `<firmware-id>` you need.

A firmware upgrade will be triggered on the device the next time it updates its LwM2M registration — which is every 10 minutes, so in the interest of time you might just reboot the device.  After it connects to the network and registers with the LwM2M server, it will start to download the image.  A couple minutes later, it will reboot with the new firmware.  Et voilà!
