#pragma once

// This is the reported manufacturer reported by the LwM2M client. It is an
// arbitrary string and will be exposed through the Horde API.
#define CLIENT_MANUFACTURER "Exploratory Engineering"

// This is the model number reported by the LwM2M client. It is an arbitrary
// string and will be exposed by the Horde API.
#define CLIENT_MODEL_NUMBER "EE-FOTA-00"

// This is the serial number reported by the LwM2M client. If you have some
// kind of serial number available you can use that, otherwise the IMEI (the
// ID for the cellular modem) or IMSI (The ID of the SIM in use)
#define CLIENT_SERIAL_NUMBER "1"

// This is the version of the firmware. This must match the versions set on the
// images uploaded via the Horde API (at https://api.nbiot.engineering/)
#define CLIENT_FIRMWARE_VER "1.0.0"

int fota_init();
