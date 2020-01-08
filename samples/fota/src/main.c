#include <zephyr.h>
#include <logging/log.h>

#include "fota.h"

LOG_MODULE_REGISTER(app_main, CONFIG_APP_LOG_LEVEL);

void main() {
	fota_client_info client_info = {
		manufacturer: "Exploratory Engineering",
		model_number: "EE-FOTA-00",
		serial_number: "1",
		firmware_version: "1.0.0",
	};

	int ret = fota_init(client_info);
	if (ret) {
		LOG_ERR("fota_init: %d", ret);
		return;
	}
}
