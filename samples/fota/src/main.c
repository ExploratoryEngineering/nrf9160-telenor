#include <zephyr.h>
#include <logging/log.h>

#include "fota.h"

LOG_MODULE_REGISTER(app_main, CONFIG_APP_LOG_LEVEL);

void main() {
	int ret = fota_init();
	if (ret) {
		LOG_ERR("fota_init: %d", ret);
		return;
	}
}
