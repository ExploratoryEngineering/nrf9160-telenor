# Firmware over the air (FOTA)

This sample demonstrates how a device can download firmware and update itself using the Lightweight Machine to Machine (LwM2M) protocol.

Build the firmware and flash it to the device by running

```sh
west build
west flash
```

Now, modify the firmware version on the command line (or by editing [prf.conf](prj.conf)), build again (but don't flash!), upload the image to the Telenor IoT Gateway, and mark the device for upgrade, as follows:

```sh
rm -rf build  # unfortunately necessary, or Zephyr will not see the version change
west build -- -DCONFIG_APP_FIRMWARE_VERSION=\"1.0.1\"
curl -HX-API-Token:<token> -F image=@build/zephyr/app_update.bin                           https://api.nbiot.telenor.io/collections/<collection-id>/firmware
curl -HX-API-Token:<token> -XPATCH -d'{"version": "1.0.1"}'                                https://api.nbiot.telenor.io/collections/<collection-id>/firmware/<firmware-id>
curl -HX-API-Token:<token> -XPATCH -d'{"firmware": {"management": "device"}}'              https://api.nbiot.telenor.io/collections/<collection-id>
curl -HX-API-Token:<token> -XPATCH -d'{"firmware": {"targetFirmwareId": "<firmware-id>"}}' https://api.nbiot.telenor.io/collections/<collection-id>/devices/<device-id>
```

with `<token>`, `<collection-id>`, `<device-id>`, and `<firmware-id>` replaced by appropriate values.  The output of the first `curl` command will give you the `<firmware-id>` you need.

A firmware upgrade will be triggered on the device the next time it updates its LwM2M registration — which is every 10 minutes, so in the interest of time you might just reboot the device.  After it connects to the network and registers with the LwM2M server, it will start to download the image.  A couple minutes later, it will reboot with the new firmware.  Et voilà!
