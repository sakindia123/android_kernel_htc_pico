/* include/linux/leds-pm8029.h
 *
 * Copyright (C) 2009 HTC Corporation.
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

#ifndef _LINUX_LEDS_PM8029_H
#define _LINUX_LEDS_PM8029_H

#include <linux/leds.h>

enum pmic8029_leds {
	PMIC8029_DRV1 = 0,
	PMIC8029_DRV2,
	PMIC8029_DRV3,
	PMIC8029_DRV4,
	PMIC8029_GPIO1,
	PMIC8029_GPIO5,
	PMIC8029_GPIO6,
	PMIC8029_GPIO8,
};

struct pm8029_led_config {
	const char *name;
	int bank;
	int init_pwm_brightness;
	int out_current;
};

struct pm8029_led_platform_data {
	struct pm8029_led_config *led_config;
	int num_leds;
};

struct pm8029_led_data {
	struct led_classdev ldev;
	struct pm8029_led_config *led_config;
	enum led_brightness brightness;
	int bank;
	int init_pwm_brightness;
	int out_current;
};

#endif /* _LINUX_LEDS_PM8029_H */
