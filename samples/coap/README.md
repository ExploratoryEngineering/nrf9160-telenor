# Constrained Application Protocol (CoAP)

This sample demonstrates how a device can act as a CoAP node (client and server).

Navigate to the Live Stream tab of the device you've registered in the [Telenor NB-IoT Service](https://nbiot.engineering/).  Then build the firmware and flash it to the device by running

```sh
west build
west flash
```

After the device boots, it may take some time before it is connected to the network.  Then, it will send a CoAP POST message upstream and start listening for downstream messages.  You should see the upstream message in the Live Stream tab.

To send a downstream message, select `CoAP` in the Message Type field, enter `message` in the Path field, enter a payload string, check the Encode Payload box, and click Send Message.  The device should report having received the POST request in its serial output.

See [the documentation](https://docs.nbiot.engineering/) for how to send and receive CoAP messages via the REST API and client libraries.
