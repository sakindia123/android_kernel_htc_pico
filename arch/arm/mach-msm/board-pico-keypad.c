/* arch/arm/mach-msm/board-pico-keypad.c
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

#include "board-pico.h"
#include "proc_comm.h"

static char *keycaps = "--qwerty";
#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "board_pico."

module_param_named(keycaps, keycaps, charp, 0);

static struct gpio_event_direct_entry pico_keypad_nav_map[] = {
	{
		.gpio = PICO_POWER_KEY,
		.code = KEY_POWER,
	},
	{
		.gpio = PICO_GPIO_VOL_DOWN,
		.code = KEY_VOLUMEDOWN,
	},
	{
		.gpio = PICO_GPIO_VOL_UP,
		.code = KEY_VOLUMEUP,
	},
};


static void pico_direct_inputs_gpio(void)
{
	int res;

	static struct msm_gpio matirx_inputs_gpio_table[] = {
		{ GPIO_CFG(PICO_POWER_KEY, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,	GPIO_CFG_4MA),
							 "power_key"  },
		{ GPIO_CFG(PICO_GPIO_VOL_DOWN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,	GPIO_CFG_4MA),
							 "volumn_down" },
		{ GPIO_CFG(PICO_GPIO_VOL_DOWN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,	GPIO_CFG_4MA),
							 "volumn_up" },
	};

	res = msm_gpios_request_enable(matirx_inputs_gpio_table,
				 ARRAY_SIZE(matirx_inputs_gpio_table));
	if (res) {
		pr_err("%s: unable to enable gpios for matirx_inputs_gpio_table\n", __func__);
		return;
	}
}


static struct gpio_event_input_info pico_keypad_power_info = {
	.info.func = gpio_event_input_func,
	.flags = GPIOEDF_PRINT_KEYS,
	.type = EV_KEY,
	.debounce_time.tv64 = 5 * NSEC_PER_MSEC,
	.keymap = pico_keypad_nav_map,
	.keymap_size = ARRAY_SIZE(pico_keypad_nav_map),
	.setup_input_gpio = pico_direct_inputs_gpio,
};

static struct gpio_event_info *pico_keypad_info[] = {
	&pico_keypad_power_info.info,
};

static struct gpio_event_platform_data pico_keypad_data = {
	.name = "pico-keypad",
	.info = pico_keypad_info,
	.info_count = ARRAY_SIZE(pico_keypad_info),
};

static struct platform_device pico_keypad_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev		= {
		.platform_data	= &pico_keypad_data,
	},
};
/*
static int pico_reset_keys_up[] = {
	KEY_VOLUMEUP,
	0
};
*/
static struct keyreset_platform_data pico_reset_keys_pdata = {
	/*.keys_up = pico_reset_keys_up,*/
	.keys_down = {
		KEY_POWER,
		KEY_VOLUMEDOWN,
		KEY_VOLUMEUP,
		0
	},
};

static struct platform_device pico_reset_keys_device = {
	.name = KEYRESET_NAME,
	.dev.platform_data = &pico_reset_keys_pdata,
};

int __init pico_init_keypad(void)
{
	if (platform_device_register(&pico_reset_keys_device))
		printk(KERN_WARNING "%s: register reset key fail\n", __func__);


	return platform_device_register(&pico_keypad_device);
}

