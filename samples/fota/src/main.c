#include <zephyr.h>
#include <fw_info.h>
#include <logging/log.h>
#include <pm_config.h>
#include <secure_services.h>
#include <fw_info.h>

#include "fota.h"

LOG_MODULE_REGISTER(app_main, CONFIG_APP_LOG_LEVEL);

void main() {
	LOG_INF("fota v2");
	// static const mcuboot_slots_t *mcuboot_slots = (mcuboot_slots_t *)&(NRF_UICR_S->OTP[0]);
	LOG_INF("addr s0 %08X, s1 %08X", PM_S0_ADDRESS, PM_S1_ADDRESS);
	struct fw_info s0_info;
	struct fw_info s1_info;
	uint8_t mcuboot_version = 0;
	uint8_t slot = 0;
	
	int ret = spm_firmware_info(PM_S0_ADDRESS, &s0_info);
	if (ret != 0) {
		LOG_ERR("Could not find firmware info for s0 (err: %d)\n", ret);
	} else {
		if (s0_info.valid == CONFIG_FW_INFO_VALID_VAL) {
			mcuboot_version = (uint8_t) s0_info.version;
		}
		LOG_INF("mcuboot s0 version: %d valid: %d", s0_info.version, s0_info.valid == CONFIG_FW_INFO_VALID_VAL);
	}

	ret = spm_firmware_info(PM_S1_ADDRESS, &s1_info);
	if (ret != 0) {
		LOG_ERR("Could not find firmware info for s1 (err: %d)\n", ret);
	} else {
		if (s1_info.valid == CONFIG_FW_INFO_VALID_VAL && s1_info.version > mcuboot_version) {
			mcuboot_version = (uint8_t) s1_info.version;
			slot = 1;
		}
		LOG_INF("mcuboot s1 version: %d valid: %d", s1_info.version, s1_info.valid == CONFIG_FW_INFO_VALID_VAL);
	}
	
	//LOG_INF("mcuboot s1 version: %d valid: %d", s1_info.version, s1_info.valid == CONFIG_FW_INFO_VALID_VAL);
	//char version[32];
	//snprintk(version, sizeof(version), "boot%d-s%d/app%s", mcuboot_version, slot, CONFIG_APP_FIRMWARE_VERSION);
	fota_client_info client_info = {
		manufacturer: "Exploratory Engineering",
		model_number: "EE-FOTA-00",
		serial_number: "1",
		firmware_version: CONFIG_APP_FIRMWARE_VERSION,
	};

	// Initialize the application and run any self-tests before calling fota_init.
	// Otherwise, if initialization or self-tests fail after an update, reboot the system and the previous firmware image will be used.

	ret = fota_init(client_info);
	if (ret) {
		LOG_ERR("fota_init: %d", ret);
		return;
	}
}
