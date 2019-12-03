#include <zephyr.h>
#include <logging/log.h>
#include <dfu/mcuboot.h>
#include <dfu/flash_img.h>
#include <flash.h>
#include <logging/log_ctrl.h>
#include <misc/reboot.h>
#include <modem_info.h>
#include <net/lwm2m.h>
#include <stdio.h>

#include "fota.h"
#include "settings.h"

LOG_MODULE_REGISTER(app_fota, CONFIG_APP_LOG_LEVEL);

#include "pm_config.h"
#define FLASH_AREA_IMAGE_SECONDARY PM_MCUBOOT_SECONDARY_ID
#define FLASH_BANK_SIZE            PM_MCUBOOT_SECONDARY_SIZE

static struct k_delayed_work reboot_work;

static void do_reboot(struct k_work *work) {
	int ret = boot_request_upgrade(BOOT_UPGRADE_TEST);
	if (ret) {
		LOG_ERR("boot_request_upgrade: %d", ret);
	}

	sys_reboot(0);
}

static int firmware_update_cb(u16_t obj_inst_id) {
	LOG_INF("Executing firmware update");

	int ret = fota_set_updating(true);
	if (ret) {
		LOG_ERR("set firmware updating state: %d", ret);
		return ret;
	}

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
	static struct flash_img_context dfu_ctx;

	int ret;

	if (bytes_downloaded == 0) {
		flash_img_init(&dfu_ctx);

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

	ret = flash_img_buffered_write(&dfu_ctx, data, data_len, last_block);
	if (ret) {
		LOG_ERR("flash_img_buffered_write: %d", ret);
		goto cleanup;
	}

	if (last_block && total_size && bytes_downloaded != total_size) {
		LOG_ERR("Early last block, downloaded %d, expecting %d", bytes_downloaded, total_size);
		ret = -EIO;
		goto cleanup;
	}

	return 0;

cleanup:
	bytes_downloaded = 0;
	percent_downloaded = 0;

	return ret;
}

static int init_lwm2m_resources() {
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

	return 0;
}

static int init_image() {
	if (fota_updating()) {
		int ret = fota_set_updating(false);
		if (ret) {
			LOG_ERR("set firmware updating state: %d", ret);
			return ret;
		}

		// If we are updating but have booted into a confirmed image, it means
		// the test reboot failed and we have reverted to the old image.
		if (boot_is_img_confirmed()) {
			LOG_ERR("Firmware update failed");
			lwm2m_engine_set_u8("5/0/5", RESULT_UPDATE_FAILED);
			return 0;
		}

		LOG_INF("Firmware update succeeded");
		lwm2m_engine_set_u8("5/0/5", RESULT_SUCCESS);
	}

	int ret = boot_write_img_confirmed();
	if (ret) {
		LOG_ERR("confirm image: %d", ret);
		return ret;
	}

	return 0;
}

static char endpoint_name[20];

static int init_endpoint_name() {
	char imei[15] = {0};

	int err = modem_info_init();
	if (err) {
		LOG_ERR("modem_info_init: %d", err);
		return err;
	}

	int len = modem_info_string_get(MODEM_INFO_IMEI, imei);
	if (len <= 0) {
		LOG_ERR("read IMEI: %d", len);
		return len;
	}

	snprintf(endpoint_name, sizeof(endpoint_name), "nrf-%s", imei);

	return 0;
}

int fota_init() {
	int ret = init_lwm2m_resources();
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
