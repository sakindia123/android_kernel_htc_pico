/* linux/arch/arm/mach-msm/board-pico-panel.c
 *
 * Copyright (c) 2011 HTC.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm/io.h>
#include <asm/mach-types.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <mach/msm_fb.h>
#include <mach/msm_iomap.h>
#include <mach/panel_id.h>
#include <mach/msm_bus_board.h>
#include <mach/debug_display.h>

#include "../devices.h"
#include "board-pico.h"
#include "../devices-msm8x60.h"
#include "../../../drivers/video/msm/mdp_hw.h"

extern int panel_type;


static void pico_panel_power(int on)
{

	PR_DISP_INFO("%s: power %s.\n", __func__, on ? "on" : "off");

	if (on) {
		gpio_set_value(PICO_GPIO_LCM_2v85_EN, 1);
		hr_msleep(20);
		gpio_set_value(PICO_GPIO_LCM_1v8_EN, 1);

		gpio_set_value(PICO_GPIO_LCD_RST_N, 0);
		hr_msleep(10);
		gpio_set_value(PICO_GPIO_LCD_RST_N, 1);
		hr_msleep(5);
	} else {
		gpio_set_value(PICO_GPIO_LCD_RST_N, 0);

		gpio_set_value(PICO_GPIO_LCM_2v85_EN, 0);

		gpio_set_value(PICO_GPIO_LCM_1v8_EN, 0);
	}
}

static int mipi_panel_power(int on)
{
	int flag_on = !!on;
	static int mipi_power_save_on;

	if (mipi_power_save_on == flag_on)
		return 0;

	mipi_power_save_on = flag_on;

	pico_panel_power(on);

	return 0;
}

enum {
	DSI_SINGLE_LANE = 1,
	DSI_TWO_LANES,
};

static int msm_fb_get_lane_config(void)
{
	int rc = DSI_SINGLE_LANE;

	PR_DISP_INFO("DSI Single Lane\n");

	return rc;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio		= 97,
	.dsi_power_save		= mipi_panel_power,
	.get_lane_config	= msm_fb_get_lane_config,
};

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 143
#define BRI_SETTING_MAX                 255

static unsigned char pico_shrink_pwm(int val)
{
	unsigned int pwm_min, pwm_default, pwm_max;
	unsigned char shrink_br = BRI_SETTING_MAX;

	if (panel_type == PANEL_ID_PIO_AUO) {
		pwm_min = 10;
		pwm_default = 115;
		pwm_max = 217;
	} else {
		pwm_min = 10;
		pwm_default = 133;
		pwm_max = 255;
	}

	if (val <= 0) {
		shrink_br = 0;
	} else if (val > 0 && (val < BRI_SETTING_MIN)) {
			shrink_br = pwm_min;
	} else if ((val >= BRI_SETTING_MIN) && (val <= BRI_SETTING_DEF)) {
			shrink_br = (val - BRI_SETTING_MIN) * (pwm_default - pwm_min) /
		(BRI_SETTING_DEF - BRI_SETTING_MIN) + pwm_min;
	} else if (val > BRI_SETTING_DEF && val <= BRI_SETTING_MAX) {
			shrink_br = (val - BRI_SETTING_DEF) * (pwm_max - pwm_default) /
		(BRI_SETTING_MAX - BRI_SETTING_DEF) + pwm_default;
	} else if (val > BRI_SETTING_MAX)
			shrink_br = pwm_max;

	//PR_DISP_INFO("brightness orig=%d, transformed=%d\n", val, shrink_br);

	return shrink_br;
}

static struct msm_panel_common_pdata mipi_pico_panel_data = {
	.shrink_pwm = NULL,
};

static struct platform_device mipi_dsi_cmd_hvga_panel_device = {
	.name = "mipi_novatek",
	.id = 0,
	.dev = {
		.platform_data = &mipi_pico_panel_data,
	}
};

static int msm_fb_detect_panel(const char *name)
{
	if (panel_type == PANEL_ID_PIO_AUO) {
		if (!strcmp(name, "mipi_cmd_novatek_hvga"))
			return 0;
	} else if ((panel_type == PANEL_ID_PIO_SAMSUNG) | (panel_type == PANEL_ID_PIO_SAMSUNG_C2)) {
		if (!strcmp(name, "mipi_cmd_samsung_hvga"))
			return 0;
	}

	PR_DISP_WARN("%s: not supported '%s'\n", __func__, name);
	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
};

static struct resource msm_fb_resources[] = {
	{
		.start = MSM_FB_BASE,
		.end = MSM_FB_BASE + MSM_FB_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = 97,
	.mdp_rev = MDP_REV_303,
};

static void __init msm_fb_add_devices(void)
{
	PR_DISP_INFO("panel ID= 0x%x\n", panel_type);
	msm_fb_register_device("mdp", &mdp_pdata);

	if (panel_type != PANEL_ID_NONE)
		msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
}

/*
TODO:
1.find a better way to handle msm_fb_resources, to avoid passing it across file.
2.error handling
 */
int __init pico_init_panel(void)
{
	int ret=0;

	if ((panel_type == PANEL_ID_PIO_SAMSUNG) | (panel_type == PANEL_ID_PIO_SAMSUNG_C2))
		mipi_dsi_cmd_hvga_panel_device.name = "mipi_samsung";
	else if (panel_type == PANEL_ID_PIO_AUO)
		mipi_dsi_cmd_hvga_panel_device.name = "mipi_novatek";
	else
		pico_panel_power(0);

	PR_DISP_INFO("panel_type= 0x%x\n", panel_type);
	PR_DISP_INFO("%s: %s\n", __func__, mipi_dsi_cmd_hvga_panel_device.name);

	mipi_pico_panel_data.shrink_pwm = pico_shrink_pwm;


	ret = platform_device_register(&msm_fb_device);
	ret = platform_device_register(&mipi_dsi_cmd_hvga_panel_device);

	msm_fb_add_devices();

	return 0;
}
