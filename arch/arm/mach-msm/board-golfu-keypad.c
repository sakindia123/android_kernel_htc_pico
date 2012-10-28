/* arch/arm/mach-msm/board-golfu-keypad.c
 * Copyright (C) 2010 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/gpio_event.h>
#include <linux/gpio.h>
#include <linux/keyreset.h>
#include <asm/mach-types.h>

#include <mach/board_htc.h>

#include "board-golfu.h"
#include "proc_comm.h"

static char *keycaps = "--qwerty";
#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "board_golfu."

module_param_named(keycaps, keycaps, charp, 0);


static struct gpio_event_direct_entry golfu_keypad_nav_map[] = {
	{
		.gpio = GOLFU_POWER_KEY,
		.code = KEY_POWER,
	},
	{
		.gpio = GOLFU_GPIO_VOL_DOWN,
		.code = KEY_VOLUMEDOWN,
	},
	{
		.gpio = GOLFU_GPIO_VOL_UP,
		.code = KEY_VOLUMEUP,
	},
};

static void golfu_direct_inputs_gpio(void)
{
	int res;

	static struct msm_gpio matirx_inputs_gpio_table[] = {
		{ GPIO_CFG(GOLFU_POWER_KEY, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,	GPIO_CFG_4MA),
							 "power_key"  },
		{ GPIO_CFG(GOLFU_GPIO_VOL_DOWN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,	GPIO_CFG_4MA),
							 "volumn_down" },
		{ GPIO_CFG(GOLFU_GPIO_VOL_DOWN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,	GPIO_CFG_4MA),
							 "volumn_up" },
	};

	res = msm_gpios_request_enable(matirx_inputs_gpio_table,
				 ARRAY_SIZE(matirx_inputs_gpio_table));
	if (res) {
		pr_err("%s: unable to enable gpios for matirx_inputs_gpio_table\n", __func__);
		return;
	}
}


static struct gpio_event_input_info golfu_keypad_power_info = {
	.info.func = gpio_event_input_func,
	.flags = GPIOEDF_PRINT_KEYS,
	.type = EV_KEY,
/*	.debounce_time.tv.nsec = 5 * NSEC_PER_MSEC, */
	.keymap = golfu_keypad_nav_map,
	.keymap_size = ARRAY_SIZE(golfu_keypad_nav_map),
	.setup_input_gpio = golfu_direct_inputs_gpio,
};

static struct gpio_event_info *golfu_keypad_info[] = {
	&golfu_keypad_power_info.info,
};

static struct gpio_event_platform_data golfu_keypad_data = {
	.name = "golfu-keypad",
	.info = golfu_keypad_info,
	.info_count = ARRAY_SIZE(golfu_keypad_info),
};

static struct platform_device golfu_keypad_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev		= {
		.platform_data	= &golfu_keypad_data,
	},
};
/*
static int golfu_reset_keys_up[] = {
	KEY_VOLUMEUP,
	0
};
*/
static struct keyreset_platform_data golfu_reset_keys_pdata = {
	/*.keys_up = golfu_reset_keys_up,*/
	.keys_down = {
		KEY_POWER,
		KEY_VOLUMEDOWN,
		KEY_VOLUMEUP,
		0
	},
};

static struct platform_device golfu_reset_keys_device = {
	.name = KEYRESET_NAME,
	.dev.platform_data = &golfu_reset_keys_pdata,
};

int __init golfu_init_keypad(void)
{
	if (platform_device_register(&golfu_reset_keys_device))
		printk(KERN_WARNING "%s: register reset key fail\n", __func__);


	return platform_device_register(&golfu_keypad_device);
}

