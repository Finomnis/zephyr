/*
 * Copyright (c) 2024 Martin Stumpf <finomnis@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>

#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

static void initialize_gui()
{
	lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_TRANSP, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(lv_layer_bottom(), LV_OPA_TRANSP, LV_PART_MAIN);
}

int main(void)
{
	int err;
	const struct device *display_dev;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return -ENODEV;
	}

	struct display_capabilities display_caps;
	display_get_capabilities(display_dev, &display_caps);
	if (!(display_caps.supported_pixel_formats | PIXEL_FORMAT_ARGB_8888)) {
		LOG_ERR("Display does not support ARGB8888 mode");
		return -ENOTSUP;
	}

	if (PIXEL_FORMAT_ARGB_8888 != display_caps.current_pixel_format) {
		err = display_set_pixel_format(display_dev, PIXEL_FORMAT_ARGB_8888);
		if (0 != err) {
			LOG_ERR("Failed to set ARGB8888 pixel format");
			return err;
		}
	}

	initialize_gui();

	lv_timer_handler();
	display_blanking_off(display_dev);

	while (1) {
		uint32_t sleep_ms = lv_timer_handler();
		k_msleep(MAX(sleep_ms, INT32_MAX));
	}

	return 0;
}
