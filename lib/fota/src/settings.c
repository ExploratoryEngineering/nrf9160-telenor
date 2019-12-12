#include <zephyr.h>
#include <logging/log.h>
#include <settings/settings.h>

#include "settings.h"

LOG_MODULE_REGISTER(app_settings, CONFIG_APP_LOG_LEVEL);

static bool updating;

static int set(const char *key, size_t len_rd, settings_read_cb read_cb, void *cb_arg) {
	if (!key) {
		return -ENOENT;
	}

	if (strcmp(key, "updating") == 0) {
		int len = read_cb(cb_arg, &updating, sizeof(updating));
		if (len < sizeof(updating)) {
			LOG_ERR("read updating state");
			updating = false;
		}

		return 0;
	}

	return -ENOENT;
}

static struct settings_handler fota_settings = {
	.name = "fota",
	.h_set = set,
};

int fota_settings_init() {
	int err = settings_subsys_init();
	if (err) {
		LOG_ERR("settings_subsys_init: %d", err);
		return err;
	}

	err = settings_register(&fota_settings);
	if (err) {
		LOG_ERR("settings_register: %d", err);
		return err;
	}

	err = settings_load_subtree("fota");
	if (err) {
		LOG_ERR("settings_load: %d", err);
		return err;
	}

	return 0;
}

bool fota_updating() {
	return updating;
}

int fota_set_updating(bool updating_) {
	updating = updating_;
	return settings_save_one("fota/updating", &updating, sizeof(updating));
}
