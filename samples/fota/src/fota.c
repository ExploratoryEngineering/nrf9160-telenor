#include <zephyr.h>
#include <bsd.h>
#include <logging/log.h>
#include <dfu/mcuboot.h>
#include <dfu/dfu_target.h>
#include <power/reboot.h>
#include <modem_info.h>
#include <net/bsdlib.h>
#include <net/lwm2m.h>
#include <stdio.h>

#include "fota.h"

LOG_MODULE_REGISTER(app_fota, CONFIG_APP_LOG_LEVEL);

#include "pm_config.h"
#define FLASH_AREA_IMAGE_SECONDARY PM_MCUBOOT_SECONDARY_ID
#define FLASH_BANK_SIZE            PM_MCUBOOT_SECONDARY_SIZE

static int check_modem_firmware_update() {
	// Modem firmware updates require a second reboot.
	int err = bsdlib_get_init_ret();
	switch (err) {
	case MODEM_DFU_RESULT_OK:
		LOG_INF("Modem firmware update successful!");
		LOG_INF("Modem will run the new firmware after reboot");
		sys_reboot(0);
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		LOG_ERR("Modem firmware update failed");
		LOG_ERR("Modem will run non-updated firmware after reboot.");
		sys_reboot(0);
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		LOG_ERR("Modem firmware update failed: %d", err);
		return err;
	}
	return 0;
}

static struct k_delayed_work reboot_work;

static void do_reboot(struct k_work *work) {
	int ret = boot_request_upgrade(BOOT_UPGRADE_TEST);
	if (ret) {
		LOG_ERR("boot_request_upgrade: %d", ret);
	}

	// removed automatic reboot to inspect flash content
	// sys_reboot(0);
}

static int firmware_update_cb(u16_t obj_inst_id) {
	LOG_INF("Executing firmware update");

	// Wait a few seconds before rebooting so that the lwm2m client has a chance
	// to acknowledge having received the Update signal.
	k_delayed_work_submit(&reboot_work, K_SECONDS(10));

	return 0;
}

static void *firmware_get_buf(u16_t obj_inst_id, u16_t res_id, u16_t res_inst_id, size_t *data_len) {
	static u8_t firmware_buf[CONFIG_LWM2M_COAP_BLOCK_SIZE];
	*data_len = sizeof(firmware_buf);
	return firmware_buf;
}

static void dfu_target_callback_handler(enum dfu_target_evt_id evt) {
	switch (evt) {
	case DFU_TARGET_EVT_TIMEOUT:
		LOG_INF("dfu_target erase pending");
		break;
	case DFU_TARGET_EVT_ERASE_DONE:
		LOG_INF("dfu_target erase done");
		break;
	default:
		LOG_ERR("dfu_target download error");
	}
}

static int firmware_block_received_cb(u16_t obj_inst_id, u16_t res_id, u16_t res_inst_id,
		u8_t *data, u16_t data_len, bool last_block, size_t total_size) {
	if (total_size > FLASH_BANK_SIZE) {
		LOG_ERR("Artifact file size too big: %d", total_size);
		return -EINVAL;
	}

	if (!data_len) {
		LOG_ERR("Data len is zero, nothing to write.");
		return -EINVAL;
	}

	static u32_t bytes_downloaded;
	static u8_t percent_downloaded;

	int ret;

	if (bytes_downloaded == 0) {
		int img_type = dfu_target_img_type(data, data_len);
		if (img_type < 0) {
			LOG_ERR("dfu_target_img_type: %d", img_type);
			return img_type;
		}

		const char *img_type_str = "";
		switch (img_type) {
		case DFU_TARGET_IMAGE_TYPE_MCUBOOT:
			img_type_str = " MCUBoot";
			break;
		case DFU_TARGET_IMAGE_TYPE_MODEM_DELTA:
			img_type_str = " modem delta";
			break;
		}
		LOG_INF("Started downloading%s image", img_type_str);

		ret = dfu_target_init(img_type, total_size, dfu_target_callback_handler);
		if (ret) {
			LOG_ERR("dfu_target_init(%d, %d): %d", img_type, total_size, ret);
			return ret;
		}

#ifndef CONFIG_IMG_ERASE_PROGRESSIVELY
		ret = boot_erase_img_bank(FLASH_AREA_IMAGE_SECONDARY);
		if (ret) {
			LOG_ERR("boot_erase_img_bank: %d", ret);
			goto cleanup;
		}
#endif
	}

	bytes_downloaded += data_len;

	u8_t percent = 100;
	if (total_size) {
		percent = 100 * bytes_downloaded / total_size;
	}
	if (percent_downloaded < percent) {
		percent_downloaded = percent;
		LOG_INF("%d%%", percent_downloaded);
	}

	ret = dfu_target_write(data, data_len);
	if (ret) {
		LOG_ERR("dfu_target_write: %d", ret);
		goto cleanup;
	}

	if (last_block && total_size && bytes_downloaded != total_size) {
		LOG_ERR("Early last block, downloaded %d, expecting %d", bytes_downloaded, total_size);
		ret = -EIO;
		goto cleanup;
	}

	if (last_block) {
		ret = dfu_target_done(true);
		if (ret) {
			LOG_ERR("dfu_target_done(true): %d", ret);
			goto cleanup;
		}
	}

	return 0;

cleanup:
	bytes_downloaded = 0;
	percent_downloaded = 0;

	int done_ret = dfu_target_done(false);
	if (done_ret) {
		LOG_ERR("dfu_target_done(false): %d", done_ret);
	}

	return ret;
}

static int init_lwm2m_resources(fota_client_info client_info) {
	LOG_INF("Firmware version: %s", log_strdup(client_info.firmware_version));
	LOG_INF("Model number:     %s", client_info.model_number);
	LOG_INF("Serial numbera:   %s", client_info.serial_number);
	LOG_INF("Manufacturer:     %s", client_info.manufacturer);

	char *server_url;
	u16_t server_url_len;
	u8_t server_url_flags;
	int ret = lwm2m_engine_get_res_data("0/0/0", (void **)&server_url, &server_url_len, &server_url_flags);
	if (ret) {
		LOG_ERR("Error getting LwM2M server URL data: %d", ret);
		return ret;
	}
	snprintk(server_url, server_url_len, "coap://172.16.15.14");

	// Security Mode (3 == NoSec)
	ret = lwm2m_engine_set_u8("0/0/2", 3);
	if (ret) {
		LOG_ERR("Error setting LwM2M security mode: %d", ret);
		return ret;
	}

	// Firmware pull uses the buffer set by the Package resource (5/0/0) pre-write callback
	// for passing downloaded firmware chunks to the firmware write callback.
	lwm2m_engine_register_pre_write_callback("5/0/0", firmware_get_buf);
	lwm2m_firmware_set_write_cb(firmware_block_received_cb);

	k_delayed_work_init(&reboot_work, do_reboot);
	lwm2m_firmware_set_update_cb(firmware_update_cb);

	lwm2m_engine_set_res_data("3/0/0", client_info.manufacturer, strlen(client_info.manufacturer), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data("3/0/1", client_info.model_number, strlen(client_info.model_number), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data("3/0/2", client_info.serial_number, strlen(client_info.serial_number), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data("3/0/3", client_info.firmware_version, strlen(client_info.firmware_version), LWM2M_RES_DATA_FLAG_RO);

	return 0;
}

static int init_image() {
	if (!boot_is_img_confirmed()) {
		int ret = boot_write_img_confirmed();
		if (ret) {
			LOG_ERR("confirm image: %d", ret);
			return ret;
		}

		LOG_INF("Confirmed firmware image.");
	}

	return 0;
}

static char endpoint_name[20];

static int init_endpoint_name() {
	int err = modem_info_init();
	if (err) {
		LOG_ERR("modem_info_init: %d", err);
		return err;
	}

	char imei[15] = {0};
	int len = modem_info_string_get(MODEM_INFO_IMEI, imei);
	if (len <= 0) {
		LOG_ERR("read IMEI: %d", len);
		return len;
	}
	snprintf(endpoint_name, sizeof(endpoint_name), "nrf-%s", imei);

	char fw_version[MODEM_INFO_MAX_RESPONSE_SIZE] = {0};
	len = modem_info_string_get(MODEM_INFO_FW_VERSION, fw_version);
	if (len <= 0) {
		LOG_ERR("read firmware version: %d", len);
		return len;
	}
	LOG_INF("Modem firmware version: %s", log_strdup(fw_version));

	return 0;
}

int fota_init(fota_client_info client_info) {
	int ret = check_modem_firmware_update();
	if (ret) {
		LOG_ERR("check_modem_firmware_update: %d", ret);
		return ret;
	}
	
	ret = init_lwm2m_resources(client_info);
	if (ret) {
		LOG_ERR("init_lwm2m_resources: %d", ret);
		return ret;
	}

	ret = init_image();
	if (ret) {
		LOG_ERR("init_image: %d", ret);
		return ret;
	}

	ret = init_endpoint_name();
	if (ret) {
		return ret;
	}

	static struct lwm2m_ctx client;
	lwm2m_rd_client_start(&client, endpoint_name, NULL);

	return 0;
}
