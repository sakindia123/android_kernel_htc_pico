/* driver/leds/leds-pm8058.c
 *
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
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds-pm8029.h>
//#include <mach/pmic.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include "../../../arch/arm/mach-msm/proc_comm.h"

#define LED_DBG_LOG(fmt, ...) \
		printk(KERN_DEBUG "[LED]" fmt, ##__VA_ARGS__)
#define LED_INFO_LOG(fmt, ...) \
		printk(KERN_INFO "[LED]" fmt, ##__VA_ARGS__)
#define LED_ERR_LOG(fmt, ...) \
		printk(KERN_ERR "[LED][ERR]" fmt, ##__VA_ARGS__)

static void pm8029_led_brightness_set(struct led_classdev *led_cdev,
					   enum led_brightness brightness)
{
	struct pm8029_led_data *ldata;
	uint32_t data1 = 0x00000002, data2 = 0x00000000;
	int rc;

	ldata = container_of(led_cdev, struct pm8029_led_data, ldev);
	data2 |= ldata->out_current;

	if (brightness && ldata->init_pwm_brightness)
		data2 |= (ldata->init_pwm_brightness << 8);
	else
		data2 |= (brightness << 8);

	data2 |= (ldata->bank << 16);

	LED_INFO_LOG("%s: data2 0x%x\n", __func__, data2);
	rc = msm_proc_comm(PCOM_CUSTOMER_CMD2, &data1, &data2);
	if (rc)
		LED_ERR_LOG("%s: data2 0x%x, rc=%d\n", __func__, data2, rc);
}

static ssize_t pm8029_led_currents_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct led_classdev *led_cdev;
	struct pm8029_led_data *ldata;

	led_cdev = (struct led_classdev *) dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct pm8029_led_data, ldev);

	return sprintf(buf, "%d\n", ldata->out_current);
}

static ssize_t pm8029_led_currents_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	int currents = 0;
	struct led_classdev *led_cdev;
	struct pm8029_led_data *ldata;

	sscanf(buf, "%d", &currents);
	if (currents < 0)
		return -EINVAL;

	led_cdev = (struct led_classdev *)dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct pm8029_led_data, ldev);

	LED_INFO_LOG("%s: bank %d currents %d +\n", __func__, ldata->bank, currents);

	ldata->out_current = currents;

	ldata->ldev.brightness_set(led_cdev, 0);
	if (currents)
		ldata->ldev.brightness_set(led_cdev, 255);

	LED_INFO_LOG("%s: bank %d currents %d -\n", __func__, ldata->bank, currents);
	return count;
}

static DEVICE_ATTR(currents, 0644, pm8029_led_currents_show,
		   pm8029_led_currents_store);

static int pm8029_led_probe(struct platform_device *pdev)
{
	struct pm8029_led_platform_data *pdata;
	struct pm8029_led_data *ldata;
	int i, ret;

	ret = -ENOMEM;

	pdata = pdev->dev.platform_data;
	if (pdata == NULL) {
		LED_ERR_LOG("%s: platform data is NULL\n", __func__);
		return -ENODEV;
	}

	ldata = kzalloc(sizeof(struct pm8029_led_data)
			* pdata->num_leds, GFP_KERNEL);
	if (!ldata && pdata->num_leds) {
		ret = -ENOMEM;
		LED_ERR_LOG("%s: failed on allocate ldata\n", __func__);
		goto err_exit;
	}

	dev_set_drvdata(&pdev->dev, ldata);

	for (i = 0; i < pdata->num_leds; i++) {
		ldata[i].ldev.name = pdata->led_config[i].name;
		ldata[i].bank = pdata->led_config[i].bank;
		ldata[i].init_pwm_brightness =  pdata->led_config[i].init_pwm_brightness;
		ldata[i].out_current =  pdata->led_config[i].out_current;

		ldata[i].ldev.brightness_set = pm8029_led_brightness_set;
		ret = led_classdev_register(&pdev->dev, &ldata[i].ldev);
		if (ret < 0) {
			LED_ERR_LOG("%s: failed on led_classdev_register [%s]\n",
				__func__, ldata[i].ldev.name);
			goto err_register_led_cdev;
		}

		if (ldata[i].bank <= PMIC8029_DRV4) {
			ret = device_create_file(ldata[i].ldev.dev, &dev_attr_currents);
				if (ret < 0) {
					LED_ERR_LOG("%s: Failed to create attr blink [%d]\n", __func__, i);
					goto err_register_attr_currents;
			}
		}
	}

	return 0;

err_register_attr_currents:
	for (i--; i >= 0; i--) {
		if (ldata[i].bank > PMIC8029_DRV4)
			continue;
		device_remove_file(ldata[i].ldev.dev, &dev_attr_currents);
	}
	i = pdata->num_leds;

err_exit:
err_register_led_cdev:
	return ret;
}

static int __devexit pm8029_led_remove(struct platform_device *pdev)
{
	struct pm8029_led_platform_data *pdata;
	struct pm8029_led_data *ldata;
	int i;

	pdata = pdev->dev.platform_data;
	ldata = platform_get_drvdata(pdev);

	for (i = 0; i < pdata->num_leds; i++) {
		if (ldata[i].bank <= PMIC8029_DRV4)
			device_remove_file(ldata[i].ldev.dev,
					   &dev_attr_currents);
	}

	kfree(ldata);

	return 0;
}

static struct platform_driver pm8029_led_driver = {
	.probe = pm8029_led_probe,
	.remove = __devexit_p(pm8029_led_remove),
	.driver = {
		   .name = "leds-pm8029",
		   .owner = THIS_MODULE,
		   },
};

int __init pm8029_led_init(void)
{
	return platform_driver_register(&pm8029_led_driver);
}

void pm8029_led_exit(void)
{
	platform_driver_unregister(&pm8029_led_driver);
}

module_init(pm8029_led_init);
module_exit(pm8029_led_exit);

MODULE_DESCRIPTION("pm8058 led driver");
MODULE_LICENSE("GPL");
