/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio_event.h>
#include <linux/cy8c_tma_ts.h>
#include <linux/himax8526a.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <mach/system.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/msm_hsusb.h>
#include <mach/htc_usb.h>
#include <mach/rpc_hsusb.h>
#include <mach/rpc_pmapp.h>
#include <mach/usbdiag.h>
#include <mach/usb_gadget_fserial.h>
#include <mach/msm_memtypes.h>
#include <mach/msm_serial_hs.h>
#include <mach/msm_serial_debugger.h>
#ifdef CONFIG_BT
#include <mach/htc_bdaddress.h>
#include <mach/htc_sleep_clk.h>
#endif
#include <linux/usb/android_composite.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <mach/vreg.h>
#include <mach/pmic.h>
#include <mach/socinfo.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <asm/mach/mmc.h>
#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>
#include <linux/gpio.h>
#include <linux/android_pmem.h>
#include <linux/bootmem.h>
#include <linux/mfd/marimba.h>
#include <mach/vreg.h>
#include <linux/power_supply.h>
#include <mach/rpc_pmapp.h>

#ifdef CONFIG_BATTERY_MSM
#include <mach/msm_battery.h>
#endif
#include <linux/smsc911x.h>
#include "devices.h"
#include "timer.h"
#include "devices-msm7x2xa.h"
#include "pm.h"
#include <mach/rpc_server_handset.h>
#include <mach/board_htc.h>
#include <asm/setup.h>
#include <linux/bma250.h>
#include <mach/htc_headset_mgr.h>
#include <mach/htc_headset_gpio.h>
#include <mach/htc_headset_pmic.h>
#include <mach/htc_battery.h>
#include <linux/tps65200.h>
#include <linux/cm3628.h>
#include <linux/leds-pm8029.h>
#include "board-pico.h"

#ifdef CONFIG_PERFLOCK_BOOT_LOCK
#include <mach/perflock.h>
#endif

#ifdef CONFIG_MSM_RESERVE_PMEM
#define PMEM_KERNEL_EBI1_SIZE	0x3A000
#define MSM_PMEM_AUDIO_SIZE	0x5B000
#endif
#define BAHAMA_SLAVE_ID_FM_ADDR         0x2A
#define BAHAMA_SLAVE_ID_QMEMBIST_ADDR   0x7B
#define FM_GPIO	83

static int config_gpio_table(uint32_t *table, int len);

static struct msm_gpio qup_i2c_gpios_io[] = {
	{ GPIO_CFG(PICO_GPIO_CAMERA_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_scl" },
	{ GPIO_CFG(PICO_GPIO_CAMERA_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA),
		"qup_sda" },
	{ GPIO_CFG(PICO_GPIO_I2C_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_scl" },
	{ GPIO_CFG(PICO_GPIO_I2C_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA),
		"qup_sda" },
};

static struct msm_gpio qup_i2c_gpios_hw[] = {
	{ GPIO_CFG(PICO_GPIO_CAMERA_SCL, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_scl" },
	{ GPIO_CFG(PICO_GPIO_CAMERA_SDA, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA),
		"qup_sda" },
	{ GPIO_CFG(PICO_GPIO_I2C_SCL, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_scl" },
	{ GPIO_CFG(PICO_GPIO_I2C_SDA, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA),
		"qup_sda" },
};

static void gsbi_qup_i2c_gpio_config(int adap_id, int config_type)
{
	int rc;

	if (adap_id < 0 || adap_id > 1)
		return;

	/* Each adapter gets 2 lines from the table */
	if (config_type)
		rc = msm_gpios_request_enable(&qup_i2c_gpios_hw[adap_id*2], 2);
	else
		rc = msm_gpios_request_enable(&qup_i2c_gpios_io[adap_id*2], 2);

	if (rc < 0)
		pr_err("QUP GPIO request/enable failed: %d\n", rc);
}

#ifdef CONFIG_I2C_HTCTOOLS
#include <linux/i2c/i2c_htc_tools.h>

static uint32_t i2c_recover_gpio_table[] = {
	GPIO_CFG(PICO_GPIO_CAMERA_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(PICO_GPIO_CAMERA_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA),
	GPIO_CFG(PICO_GPIO_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(PICO_GPIO_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA),
};

static uint32_t i2c_normal_gpio_table[] = {
	GPIO_CFG(PICO_GPIO_CAMERA_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(PICO_GPIO_CAMERA_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA),
	GPIO_CFG(PICO_GPIO_I2C_SCL, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(PICO_GPIO_I2C_SDA, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA),
};

static void setupHtcPlatformData(int adap_id, struct msm_i2c_platform_data *pdata)
{
	struct htc_i2c_platform_data *htc_data = 0;
	printk(KERN_INFO "[QUP]: setting up hTC I2C tools....\n");

	htc_data = kzalloc(sizeof(struct htc_i2c_platform_data), GFP_KERNEL);
	if (!htc_data) {
		goto err_alloc_htc_data_failed;
	}
	pdata->htc_pdata = htc_data;

	htc_data->recoverSetting = &i2c_recover_gpio_table[adap_id*2];

	htc_data->operatingSetting = &i2c_normal_gpio_table[adap_id*2];

	switch (adap_id) {
	case 0:
		htc_data->gpio_clk = PICO_GPIO_CAMERA_SCL;
		htc_data->gpio_data = PICO_GPIO_CAMERA_SDA;
		break;
	case 1:
		htc_data->gpio_clk = PICO_GPIO_I2C_SCL;
		htc_data->gpio_data = PICO_GPIO_I2C_SDA;
		break;
	}

	htc_data->allocDone = 1;

	return;

err_alloc_htc_data_failed:
	return;
}

void freeHtcPlatformData(struct msm_i2c_platform_data *pdata)
{
	if (pdata)
		if (pdata->htc_pdata)
			kfree(pdata->htc_pdata);
}

#endif

static struct msm_i2c_platform_data msm_gsbi0_qup_i2c_pdata = {
// HTC_START
// from 100k to 400k
	.clk_freq		= 400000, 
// HTC_END	
	.clk			= "gsbi_qup_clk",
	.pclk			= "gsbi_qup_pclk",
	.msm_i2c_config_gpio	= gsbi_qup_i2c_gpio_config,
#ifdef CONFIG_I2C_HTCTOOLS
	.setupHtcPlatformData	= setupHtcPlatformData,
	.freeHtcPlatformData	= freeHtcPlatformData,
#endif
};

static struct msm_i2c_platform_data msm_gsbi1_qup_i2c_pdata = {
	.clk_freq		= 384000,
	.clk			= "gsbi_qup_clk",
	.pclk			= "gsbi_qup_pclk",
	.msm_i2c_config_gpio	= gsbi_qup_i2c_gpio_config,
#ifdef CONFIG_I2C_HTCTOOLS
	.setupHtcPlatformData	= setupHtcPlatformData,
	.freeHtcPlatformData	= freeHtcPlatformData,
#endif
};

#ifdef CONFIG_ARCH_MSM7X27A
#ifdef CONFIG_MSM_RESERVE_PMEM
#define MSM_PMEM_MDP_SIZE       0x1DD1000
#define MSM_PMEM_ADSP_SIZE      0x1000000
#endif
#endif

#ifdef CONFIG_USB_EHCI_MSM_72K
static int  msm_hsusb_vbus_init(int on)
{
	return 0;
}

static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{
	return;
}

static struct msm_usb_host_platform_data msm_usb_host_pdata = {
	.phy_info       = (USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM),
	.vbus_init	= msm_hsusb_vbus_init,
};
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static int hsusb_rpc_connect(int connect)
{
	if (connect)
		return msm_hsusb_rpc_connect();
	else
		return msm_hsusb_rpc_close();
}

static struct msm_otg_platform_data msm_otg_pdata = {
#ifdef CONFIG_EHCI_MSM
	.vbus_power		 = msm_hsusb_vbus_power,
#endif
	.rpc_connect		 = hsusb_rpc_connect,
	.core_clk		 = 1,
	.pemp_level		 = PRE_EMPHASIS_WITH_20_PERCENT,
	.cdr_autoreset		 = CDR_AUTO_RESET_DISABLE,
	.drv_ampl		 = HS_DRV_AMPLITUDE_DEFAULT,
	.se1_gating		 = SE1_GATING_DISABLE,
};
#endif

#ifdef CONFIG_USB_ANDROID
static int pico_phy_init_seq[] =
{
	0x2C, 0x31,
	0x08, 0x32,
	0x1D, 0x0D,
	0x1D, 0x10,
	-1
};

/* TODO
static void pico_phy_reset(void)
{
	int ret;

	printk(KERN_INFO "msm_hsusb_phy_reset\n");
	ret = msm_proc_comm(PCOM_MSM_HSUSB_PHY_RESET,
			NULL, NULL);
	if (ret)
		printk(KERN_INFO "%s failed\n", __func__);
}
*/

static struct msm_hsusb_platform_data msm_hsusb_pdata = {
	.phy_init_seq		= pico_phy_init_seq,
/* TODO
	.phy_reset		= pico_phy_reset,
*/
	.usb_id_pin_gpio	= PICO_GPIO_USB_ID_PIN,
};

static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns		= 1,
	.vendor		= "HTC",
	.product	= "Android Phone",
	.release	= 0x0100,
};

static struct platform_device usb_mass_storage_device = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &mass_storage_pdata,
	},
};

static struct htc_battery_platform_data htc_battery_pdev_data = {
	.guage_driver = GUAGE_MODEM,
	.charger = SWITCH_CHARGER_TPS65200,
	.m2a_cable_detect = 1,
};

static struct platform_device htc_battery_pdev = {
	.name = "htc_battery",
	.id = -1,
	.dev	= {
		.platform_data = &htc_battery_pdev_data,
	},
};

static struct tps65200_platform_data tps65200_data = {
	.gpio_chg_int = MSM_GPIO_TO_INT(PICO_GPIO_CHG_INT),
};

static struct i2c_board_info i2c_tps65200_devices[] = {
	{
		I2C_BOARD_INFO("tps65200", 0xD4 >> 1),
		.platform_data = &tps65200_data,
	},
};


static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id		= 0x0bb4,
	.product_id		= 0x0cc9,
	.version		= 0x0100,
	.product_name		= "Android Phone",
	.manufacturer_name	= "HTC",
	.num_products		= ARRAY_SIZE(usb_products),
	.products		= usb_products,
	.num_functions		= ARRAY_SIZE(usb_functions_all),
	.functions		= usb_functions_all,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};

void pico_add_usb_devices(void)
{
	printk(KERN_INFO "%s\n", __func__);
	android_usb_pdata.products[0].product_id =
		android_usb_pdata.product_id;
	msm_hsusb_pdata.serial_number = board_serialno();
	android_usb_pdata.serial_number = board_serialno();
	if (msm_hsusb_pdata.usb_id_pin_gpio != 0)
		android_usb_pdata.usb_id_pin_gpio = msm_hsusb_pdata.usb_id_pin_gpio;

#ifdef CONFIG_USB_MSM_OTG_72K
	msm_device_otg.dev.platform_data = &msm_otg_pdata;
	platform_device_register(&msm_device_otg);
#endif

#ifdef CONFIG_USB_EHCI_MSM_72K
	msm_device_hsusb_host.dev.platform_data = &msm_usb_host_pdata;
	platform_device_register(&msm_device_hsusb_host);
#endif

	msm_device_gadget_peripheral.dev.platform_data = &msm_hsusb_pdata;
	platform_device_register(&msm_device_gadget_peripheral);
	platform_device_register(&usb_mass_storage_device);
	platform_device_register(&android_usb_device);
}
#endif


#ifdef CONFIG_SERIAL_MSM_HS
static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
	.inject_rx_on_wakeup = 0,
	.cpu_lock_supported = 1,

	/* for bcm */
	.bt_wakeup_pin_supported = 1,
	.bt_wakeup_pin = PICO_GPIO_BT_CHIP_WAKE,
	.host_wakeup_pin = PICO_GPIO_BT_HOST_WAKE,
};
#endif

#ifdef CONFIG_BT
static struct platform_device pico_rfkill = {
	.name = "pico_rfkill",
	.id = -1,
};

static struct htc_sleep_clk_platform_data htc_slp_clk_data = {
	/*.sleep_clk_pin = PICO_WIFI_BT_SLEEP_CLK,*/

};

static struct platform_device wifi_bt_slp_clk = {
	.name = "htc_slp_clk",
	.id = -1,
	.dev = {
		.platform_data = &htc_slp_clk_data,
	},
};
#endif

static struct pm8029_led_config pm_led_config[] = {
	{
		.name = "button-backlight",
		.bank = PMIC8029_GPIO1,
		.init_pwm_brightness = 200,
	},
};

static struct pm8029_led_platform_data pm8029_leds_data = {
	.led_config = pm_led_config,
	.num_leds = ARRAY_SIZE(pm_led_config),
};

static struct platform_device pm8029_leds = {
	.name   = "leds-pm8029",
	.id     = -1,
	.dev    = {
		.platform_data  = &pm8029_leds_data,
	},
};

static struct msm_pm_platform_data msm7x27a_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 16000,
					.residency = 20000,
	},
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 12000,
					.residency = 20000,
	},
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 0,
					.suspend_enabled = 1,
					.latency = 2000,
					.residency = 0,
	},
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 2,
					.residency = 0,
	},
};

static uint32_t headset_gpio_table[] = {
	GPIO_CFG(PICO_GPIO_AUD_HP_DET, 0, GPIO_CFG_INPUT,
		 GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(PICO_GPIO_AUD_REMO_PRES, 0, GPIO_CFG_INPUT,
		 GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

/* HTC_HEADSET_GPIO Driver */
static struct htc_headset_gpio_platform_data htc_headset_gpio_data = {
	.hpin_gpio		= PICO_GPIO_AUD_HP_DET,
	.key_gpio		= PICO_GPIO_AUD_REMO_PRES,
	.key_enable_gpio	= 0,
	.mic_select_gpio	= 0,
};

static struct platform_device htc_headset_gpio = {
	.name	= "HTC_HEADSET_GPIO",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_gpio_data,
	},
};

/* HTC_HEADSET_PMIC Driver */
static struct htc_headset_pmic_platform_data htc_headset_pmic_data = {
	.driver_flag		= DRIVER_HS_PMIC_RPC_KEY,
	.hpin_gpio		= 0,
	.hpin_irq		= 0,
	.key_gpio		= 0,
	.key_irq		= 0,
	.key_enable_gpio	= 0,
	.adc_mic		= 0,
	.adc_remote		= {0, 2473, 2474, 6361, 6362, 13138},
	.hs_controller		= 0,
	.hs_switch		= 0,
};

static struct htc_headset_pmic_platform_data htc_headset_pmic_data_xd = {
	.driver_flag		= DRIVER_HS_PMIC_RPC_KEY,
	.hpin_gpio		= 0,
	.hpin_irq		= 0,
	.key_gpio		= 0,
	.key_irq		= 0,
	.key_enable_gpio	= 0,
	.adc_mic		= 0,
	.adc_remote		= {0, 2253, 2254, 5631, 5632, 13138},
	.hs_controller		= 0,
	.hs_switch		= 0,
};

static struct platform_device htc_headset_pmic = {
	.name	= "HTC_HEADSET_PMIC",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_pmic_data,
	},
};

/* HTC_HEADSET_MGR Driver */
static struct platform_device *headset_devices[] = {
	&htc_headset_pmic,
	&htc_headset_gpio,
	/* Please put the headset detection driver on the last */
};

static struct headset_adc_config htc_headset_mgr_config[] = {
	{
		.type = HEADSET_MIC,
		.adc_max = 57616,
		.adc_min = 22521,
	},
	{
		.type = HEADSET_NO_MIC,
		.adc_max = 22520,
		.adc_min = 0,
	},
};

static struct headset_adc_config htc_headset_mgr_config_xd[] = {
	{
		.type = HEADSET_MIC,
		.adc_max = 64560,
		.adc_min = 51237,
	},
	{
		.type = HEADSET_BEATS,
		.adc_max = 51236,
		.adc_min = 35661,
	},
	{
		.type = HEADSET_BEATS_SOLO,
		.adc_max = 35660,
		.adc_min = 13139,
	},
	{
		.type = HEADSET_NO_MIC,
		.adc_max = 13138,
		.adc_min = 0,
	},
};

static struct htc_headset_mgr_platform_data htc_headset_mgr_data = {
	.driver_flag		= DRIVER_HS_MGR_RPC_SERVER,
	.headset_devices_num	= ARRAY_SIZE(headset_devices),
	.headset_devices	= headset_devices,
	.headset_config_num	= ARRAY_SIZE(htc_headset_mgr_config),
	.headset_config		= htc_headset_mgr_config,
};

static struct platform_device htc_headset_mgr = {
	.name	= "HTC_HEADSET_MGR",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_mgr_data,
	},
};

static void headset_init(void)
{
	pr_info("[HS_BOARD] (%s) system_rev = %d\n", __func__, system_rev);
	config_gpio_table(headset_gpio_table, ARRAY_SIZE(headset_gpio_table));

	if (system_rev > 2) {
		pr_info("[HS_BOARD] (%s) Enable Beats support\n", __func__);
		htc_headset_pmic.dev.platform_data = &htc_headset_pmic_data_xd;
		htc_headset_mgr_data.headset_config_num =
			ARRAY_SIZE(htc_headset_mgr_config_xd);
		htc_headset_mgr_data.headset_config =
			htc_headset_mgr_config_xd;
	}
}

static struct bma250_platform_data gsensor_bma250_platform_data = {
	.intr = PICO_GPIO_GSENSORS_INT,
	.chip_layout = 0,
	.layouts = PICO_LAYOUTS,
};

static struct i2c_board_info i2c_bma250_devices[] = {
	{
		I2C_BOARD_INFO(BMA250_I2C_NAME_REMOVE_ECOMPASS, \
				0x30 >> 1),
		.platform_data = &gsensor_bma250_platform_data,
		.irq = MSM_GPIO_TO_INT(PICO_GPIO_GSENSORS_INT),
	},
};

static uint32_t pl_sensor_gpio_table[] = {
	GPIO_CFG(PICO_GPIO_PROXIMITY_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
};

static void pl_sensor_init(void)
{
	config_gpio_table(pl_sensor_gpio_table,
			  ARRAY_SIZE(pl_sensor_gpio_table));
}

static struct cm3628_platform_data cm3628_pdata = {
	.intr = PICO_GPIO_PROXIMITY_INT,
	.levels = {1, 3, 5, 17, 29, 714, 1231, 1497, 1762, 65535},
	.golden_adc = 0x378,
	.power = NULL,
	.ALS_slave_address = 0xC0 >> 1,
	.PS_slave_address = 0xC2 >> 1,
	.check_interrupt_add = 0x2C >> 1,
	.is_cmd = CM3628_ALS_IT_400ms | CM3628_ALS_PERS_2,
	.ps_thd_set = 0x4,
	.ps_conf2_val = 0,
	.ps_calibration_rule = 1,
	.ps_conf1_val = CM3628_PS_DR_1_320 | CM3628_PS_IT_1_3T,
	.ps_thd_no_cal = 0x90,
	.ps_thd_with_cal = 0x4,
	.ps_adc_offset = 0x3,
};

static struct i2c_board_info i2c_CM3628_devices[] = {
	{
		I2C_BOARD_INFO(CM3628_I2C_NAME, 0xC0 >> 1),
		.platform_data = &cm3628_pdata,
		.irq = MSM_GPIO_TO_INT(PICO_GPIO_PROXIMITY_INT),
	},
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
#ifndef CONFIG_MSM_RESERVE_PMEM
	.start = MSM_PMEM_ADSP_BASE,
	.size = MSM_PMEM_ADSP_SIZE,
#endif
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};

#ifdef CONFIG_MSM_RESERVE_PMEM
static unsigned pmem_mdp_size = MSM_PMEM_MDP_SIZE;
static int __init pmem_mdp_size_setup(char *p)
{
	pmem_mdp_size = memparse(p, NULL);
	return 0;
}

early_param("pmem_mdp_size", pmem_mdp_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
static int __init pmem_adsp_size_setup(char *p)
{
	pmem_adsp_size = memparse(p, NULL);
	return 0;
}

early_param("pmem_adsp_size", pmem_adsp_size_setup);


#endif


#define SND(desc, num) { .name = #desc, .id = num }
static struct snd_endpoint snd_endpoints_list[] = {
	SND(HANDSET, 0),
	SND(SPEAKER, 1),
	SND(HEADSET,2),
	SND(BT, 3),
	SND(CARKIT, 3),
	SND(TTY_FULL, 5),
	SND(TTY_HEADSET, 5),
	SND(TTY_VCO, 6),
	SND(TTY_HCO, 7),
	SND(NO_MIC_HEADSET, 8),
	SND(FM_HEADSET, 9),
	SND(HEADSET_AND_SPEAKER, 10),
	SND(STEREO_HEADSET_AND_SPEAKER, 10),
	SND(BT_EC_OFF, 44),
	SND(CURRENT, 256),
};
#undef SND

static struct msm_snd_endpoints msm_device_snd_endpoints = {
	.endpoints = snd_endpoints_list,
	.num = sizeof(snd_endpoints_list) / sizeof(struct snd_endpoint)
};

static struct platform_device msm_device_snd = {
	.name = "msm_snd",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_snd_endpoints
	},
};

static struct android_pmem_platform_data android_pmem_audio_pdata = {
	.name = "pmem_audio",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_audio_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_audio_pdata },
};

#define DEC0_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC1_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC2_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC3_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC4_FORMAT (1<<MSM_ADSP_CODEC_MIDI)

static unsigned int dec_concurrency_table[] = {
	/* Audio LP */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DMA)), 0,
	0, 0, 0,

	/* Concurrency 1 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	 /* Concurrency 2 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 3 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 4 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 5 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 6 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	0, 0, 0, 0,

	/* Concurrency 7 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),
};

#define DEC_INFO(name, queueid, decid, nr_codec) { .module_name = name, \
	.module_queueid = queueid, .module_decid = decid, \
	.nr_codec_support = nr_codec}

static struct msm_adspdec_info dec_info_list[] = {
	DEC_INFO("AUDPLAY0TASK", 13, 0, 11), /* AudPlay0BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY1TASK", 14, 1, 11),  /* AudPlay1BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY2TASK", 15, 2, 11),  /* AudPlay2BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY3TASK", 16, 3, 11),  /* AudPlay3BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY4TASK", 17, 4, 1),  /* AudPlay4BitStreamCtrlQueue */
};

static struct msm_adspdec_database msm_device_adspdec_database = {
	.num_dec = ARRAY_SIZE(dec_info_list),
	.num_concurrency_support = (ARRAY_SIZE(dec_concurrency_table) / \
					ARRAY_SIZE(dec_info_list)),
	.dec_concurrency_table = dec_concurrency_table,
	.dec_info_list = dec_info_list,
};

static struct platform_device msm_device_adspdec = {
	.name = "msm_adspdec",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_adspdec_database
	},
};

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
#ifndef CONFIG_MSM_RESERVE_PMEM
	.start = MSM_PMEM_MDP_BASE,
	.size = MSM_PMEM_MDP_SIZE,
#endif
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
};
static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

#ifdef CONFIG_BATTERY_MSM
static u32 msm_calculate_batt_capacity(u32 current_voltage);

static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.voltage_min_design     = 2800,
	.voltage_max_design     = 4300,
	.avail_chg_sources      = AC_CHG | USB_CHG ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
	.calculate_capacity     = &msm_calculate_batt_capacity,
};

static u32 msm_calculate_batt_capacity(u32 current_voltage)
{
	u32 low_voltage	 = msm_psy_batt_data.voltage_min_design;
	u32 high_voltage = msm_psy_batt_data.voltage_max_design;

	return (current_voltage - low_voltage) * 100
			/ (high_voltage - low_voltage);
}

static struct platform_device msm_batt_device = {
	.name               = "msm-battery",
	.id                 = -1,
	.dev.platform_data  = &msm_psy_batt_data,
};
#endif

#ifdef CONFIG_MSM_CAMERA
// HTC_START
#define CAM_I2C_CLK 60
#define CAM_I2C_DATA 61
#define CAM_RESET 125
#define CAM_STANDBY 126
#define CAM_MCLK 15
// HTC_END
// 60   i2c scl
// 61   i2c sda
// 125  reset
// 126  standby (high active)
// 15   mclk
static uint32_t camera_off_gpio_table[] = {
/*
	GPIO_CFG(61, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(60, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
*/
// HTC_START
// sleep status
	GPIO_CFG(CAM_I2C_DATA,  1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CAM_I2C_CLK,   1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CAM_RESET,     0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CAM_STANDBY,   0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CAM_MCLK,      0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),
// HTC_END
};

static uint32_t camera_on_gpio_table[] = {
/*
	GPIO_CFG(61, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(60, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(15, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
*/
// HTC_START
// sleep status
	GPIO_CFG(CAM_I2C_DATA,  1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CAM_I2C_CLK,   1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CAM_RESET,     0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CAM_STANDBY,   0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CAM_MCLK,      1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),
// HTC_END
};

#ifdef CONFIG_MSM_CAMERA_FLASH
static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
	._fsrc.current_driver_src.led1 = GPIO_SURF_CAM_GP_LED_EN1,
	._fsrc.current_driver_src.led2 = GPIO_SURF_CAM_GP_LED_EN2,
};
#endif

//HTC_START
//For pico camera power control
static struct vreg *vreg_wlan4;
static struct vreg *vreg_usim;
static struct vreg *vreg_bt;
static void pico_camera_vreg_config(int vreg_en)
{
	int rc;

	printk("pico_camera_vreg_config %d\n", vreg_en);

	if (vreg_wlan4 == NULL) { //D1.8v
		vreg_wlan4 = vreg_get(NULL, "wlan4");
		if (IS_ERR(vreg_wlan4)) {
			pr_err("%s: vreg_get(%s) failed (%ld)\n",
				__func__, "wlan4", PTR_ERR(vreg_wlan4));
			return;
		}

		rc = vreg_set_level(vreg_wlan4, 1800);
		if (rc) {
			pr_err("%s: WLAN4 set_level failed (%d)\n",
				__func__, rc);
		}
	}

	if (vreg_usim == NULL) { //IO1.8v
		// for XB/XC/... PCBA
		switch (system_rev)
		{
			case 0:
				vreg_usim = vreg_get(NULL, "usim");
				break;
			default:
				vreg_usim = vreg_get(NULL, "usim2");
				break;
		}

		if (IS_ERR(vreg_usim)) {
			pr_err("%s: vreg_get(%s) failed (%ld)\n",
				__func__, "usim", PTR_ERR(vreg_usim));
			return;
		}

		rc = vreg_set_level(vreg_usim, 1800);
		if (rc) {
			pr_err("%s: USIM set level failed (%d)\n",
				__func__, rc);
		}
	}

	if (vreg_bt == NULL) { //A2.85v
		vreg_bt = vreg_get(NULL, "bt");
		if (IS_ERR(vreg_bt)) {
			pr_err("%s: vreg_get(%s) failed (%ld)\n",
				__func__, "bt", PTR_ERR(vreg_bt));
			return;
		}

		rc = vreg_set_level(vreg_bt, 2850);
		if (rc) {
			pr_err("%s: BT set level failed (%d)\n",
				__func__, rc);
		}
	}

	if (vreg_en) {
		rc = vreg_enable(vreg_usim);
		if (rc) {
			pr_err("%s: USIM enable failed (%d)\n",
				__func__, rc);
		}
	
		hr_msleep(5);
	
		rc = vreg_enable(vreg_wlan4);
		if (rc) {
			pr_err("%s: WLAN4 enable failed (%d)\n",
				__func__, rc);
		}

		hr_msleep(5);

		rc = vreg_enable(vreg_bt);
		if (rc) {
			pr_err("%s: BT enable failed (%d)\n",
				__func__, rc);
		}

		hr_msleep(5);
	} else {
		rc = vreg_disable(vreg_bt);
		if (rc) {
			pr_err("%s: BT disable failed (%d)\n",
				__func__, rc);
		}

		hr_msleep(5);
		
		rc = vreg_disable(vreg_wlan4);
		if (rc) {
			pr_err("%s: WLAN4 disable failed (%d)\n",
				__func__, rc);
		}

		hr_msleep(5);
		
		rc = vreg_disable(vreg_usim);
		if (rc) {
			pr_err("%s: USIM disable failed (%d)\n",
				__func__, rc);
		}
	}


}
//HTC_END


static struct vreg *vreg_gp2;
static struct vreg *vreg_gp3;
static void msm_camera_vreg_config(int vreg_en)
{
	int rc;

	if (vreg_gp2 == NULL) {
		vreg_gp2 = vreg_get(NULL, "gp2");
		if (IS_ERR(vreg_gp2)) {
			pr_err("%s: vreg_get(%s) failed (%ld)\n",
				__func__, "gp2", PTR_ERR(vreg_gp2));
			return;
		}

		rc = vreg_set_level(vreg_gp2, 2850);
		if (rc) {
			pr_err("%s: GP2 set_level failed (%d)\n",
				__func__, rc);
		}
	}

	if (vreg_gp3 == NULL) {
		vreg_gp3 = vreg_get(NULL, "usb2");
		if (IS_ERR(vreg_gp3)) {
			pr_err("%s: vreg_get(%s) failed (%ld)\n",
				__func__, "gp3", PTR_ERR(vreg_gp3));
			return;
		}

		rc = vreg_set_level(vreg_gp3, 1800);
		if (rc) {
			pr_err("%s: GP3 set level failed (%d)\n",
				__func__, rc);
		}
	}

	if (vreg_en) {
		rc = vreg_enable(vreg_gp2);
		if (rc) {
			pr_err("%s: GP2 enable failed (%d)\n",
				__func__, rc);
		}

		rc = vreg_enable(vreg_gp3);
		if (rc) {
			pr_err("%s: GP3 enable failed (%d)\n",
				__func__, rc);
		}
	} else {
		rc = vreg_disable(vreg_gp2);
		if (rc) {
			pr_err("%s: GP2 disable failed (%d)\n",
				__func__, rc);
		}

		rc = vreg_disable(vreg_gp3);
		if (rc) {
			pr_err("%s: GP3 disable failed (%d)\n",
				__func__, rc);
		}
	}
}

static int config_gpio_table(uint32_t *table, int len)
{
	int rc = 0, i = 0;

	for (i = 0; i < len; i++) {
		rc = gpio_tlmm_config(table[i], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s not able to get gpio\n", __func__);
			for (i--; i >= 0; i--)
				gpio_tlmm_config(camera_off_gpio_table[i],
							GPIO_CFG_ENABLE);
			break;
		}
	}
	return rc;
}

/* HTC_START Harvey 20110804 */
/* Camera reset pin control */
static struct msm_camera_sensor_info msm_camera_sensor_mt9t013_data;

static void set_reset_pin(struct msm_camera_sensor_info *data, int value)
{
	int rc;

	rc = gpio_request(data->sensor_reset, "mt9t013");
	if (!rc) {
		gpio_direction_output(data->sensor_reset, value);
		gpio_free(data->sensor_reset);
	} else{
		pr_err("[CAM] mt9t013_init.set_reset_pin failed!\n");
	}
}
/* HTC_END */

static int config_camera_on_gpios_rear(void)
{
	int rc = 0;

	if (machine_is_msm7x27a_ffa())
		msm_camera_vreg_config(1);

//HTC_START
//Waiting HW to confirm pico need cut off power.
#ifdef CONFIG_ARCH_MSM7X27A
	pico_camera_vreg_config(1);
#endif
//HTC_END

	rc = config_gpio_table(camera_on_gpio_table,
			ARRAY_SIZE(camera_on_gpio_table));
	if (rc < 0) {
		pr_err("%s: CAMSENSOR gpio table request"
		"failed\n", __func__);
		return rc;
	}

	/* HTC_START Harvey 20110804 */
	/* The first reset pin pull up for matching the power sequence. */
	set_reset_pin(&msm_camera_sensor_mt9t013_data, 1);
	hr_msleep(5);
	/* HTC_END */
	return rc;
}

static void config_camera_off_gpios_rear(void)
{
	if (machine_is_msm7x27a_ffa())
		msm_camera_vreg_config(0);

//HTC_START
//Follow HW's IO control designed sequence. (RST-> A2.85v -> D1.8v -> IO1.8v -> MCLK & SCL/SDA)
//Waiting HW to confirm pico need cut off power.
#ifdef CONFIG_ARCH_MSM7X27A
	pico_camera_vreg_config(0);
#endif
//HTC_END

	config_gpio_table(camera_off_gpio_table,
			ARRAY_SIZE(camera_off_gpio_table));
}


struct msm_camera_device_platform_data msm_camera_device_data_rear = {
	.camera_gpio_on  = config_camera_on_gpios_rear,
	.camera_gpio_off = config_camera_off_gpios_rear,
	.ioext.csiphy = 0xA1000000,
	.ioext.csisz  = 0x00100000,
	.ioext.csiirq = INT_CSI_IRQ_1,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 192000000,
	.ioext.appphy = MSM_CLK_CTL_PHYS,
	.ioext.appsz  = MSM_CLK_CTL_SIZE,
};

#ifdef CONFIG_MT9T013
static struct msm_camera_sensor_platform_info mt9t013_sensor_7627a_info = {
	.mount_angle = 0
};
// PG-POWER_SEQ-00-{
static struct msm_camera_sensor_flash_data flash_mt9t013 = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src  = &msm_flash_src
#endif
};
// PG-POWER_SEQ-00-}
static struct msm_camera_sensor_info msm_camera_sensor_mt9t013_data = {
	.sensor_name    = "mt9t013",
	.sensor_reset   = 125,
	.sensor_pwd     = 126,
	.mclk           = 15, // PG-POWER_SEQ-00
//	.vcm_pwd        = 1,
//	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data_rear,
	.flash_data     = &flash_mt9t013, // PG-POWER_SEQ-00
	.sensor_platform_info   = &mt9t013_sensor_7627a_info,
	.csi_if         = 1
};

static struct platform_device msm_camera_sensor_mt9t013 = {
	.name      = "msm_camera_mt9t013",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9t013_data,
	},
};
#endif

static uint opt_disable_uart3;
module_param_named(disable_uart3, opt_disable_uart3, uint, 0);

static struct i2c_board_info i2c_camera_devices[] = {
	#ifdef CONFIG_MT9T013
	{
		I2C_BOARD_INFO("mt9t013", 0x6C ),// PG-POWER_SEQ-00
	},
	#endif
};
#endif

#ifdef CONFIG_PERFLOCK_BOOT_LOCK
static unsigned pico_perf_acpu_table[] = {
       245760000,
       480000000,
       600000000,
};

static struct perflock_platform_data holiday_perflock_data = {
       .perf_acpu_table = pico_perf_acpu_table,
       .table_size = ARRAY_SIZE(pico_perf_acpu_table),
};
#endif

static struct resource ram_console_resources[] = {
	{
		.start  = MSM_RAM_CONSOLE_BASE,
		.end    = MSM_RAM_CONSOLE_BASE + MSM_RAM_CONSOLE_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
};

static struct platform_device ram_console_device = {
	.name           = "ram_console",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(ram_console_resources),
	.resource       = ram_console_resources,
};

static struct platform_device *pico_devices[] __initdata = {
	&ram_console_device,
	&msm_device_dmov,
	&msm_device_smd,
	&msm_device_uart1,
	&msm_device_uart3,
	/*&msm_device_uart_dm1,*/
	&msm_device_nand,
	&msm_gsbi0_qup_i2c_device,
	&msm_gsbi1_qup_i2c_device,
	&htc_battery_pdev,
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
	&msm_device_snd,
	&usb_gadget_fserial_device,
	&msm_device_adspdec,
#ifdef CONFIG_BATTERY_MSM
	&msm_batt_device,
#endif
	&htc_headset_mgr,
#ifdef CONFIG_S5K4E1
	&msm_camera_sensor_s5k4e1,
#endif
#ifdef CONFIG_IMX072
	&msm_camera_sensor_imx072,
#endif
#ifdef CONFIG_WEBCAM_OV9726
	&msm_camera_sensor_ov9726,
#endif
#ifdef CONFIG_MT9E013
	&msm_camera_sensor_mt9e013,
#endif
	&msm_kgsl_3d0,
#ifdef CONFIG_BT
	//&msm_bt_power_device,
#endif
#ifdef CONFIG_MT9T013
	&msm_camera_sensor_mt9t013,
#endif
#ifdef CONFIG_BT
	&wifi_bt_slp_clk,
	&pico_rfkill,
	&msm_device_uart_dm1,
#endif
	&pm8029_leds,
};

#ifdef CONFIG_MSM_RESERVE_PMEM

static unsigned pmem_kernel_ebi1_size = PMEM_KERNEL_EBI1_SIZE;
static int __init pmem_kernel_ebi1_size_setup(char *p)
{
	pmem_kernel_ebi1_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_kernel_ebi1_size", pmem_kernel_ebi1_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;
static int __init pmem_audio_size_setup(char *p)
{
	pmem_audio_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_audio_size", pmem_audio_size_setup);

static struct memtype_reserve msm7x27a_reserve_table[] __initdata = {
	[MEMTYPE_SMI] = {
	},
	[MEMTYPE_EBI0] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
	[MEMTYPE_EBI1] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
};


static void __init size_pmem_devices(void)
{
#ifdef CONFIG_ANDROID_PMEM
	android_pmem_adsp_pdata.size = pmem_adsp_size;
	android_pmem_pdata.size = pmem_mdp_size;
	android_pmem_audio_pdata.size = pmem_audio_size;
#endif
}

static void __init reserve_memory_for(struct android_pmem_platform_data *p)
{
	msm7x27a_reserve_table[p->memory_type].size += p->size;
}

static void __init reserve_pmem_memory(void)
{
#ifdef CONFIG_ANDROID_PMEM
	reserve_memory_for(&android_pmem_adsp_pdata);
	reserve_memory_for(&android_pmem_pdata);
	reserve_memory_for(&android_pmem_audio_pdata);
	msm7x27a_reserve_table[MEMTYPE_EBI1].size += pmem_kernel_ebi1_size;
#endif
}

static void __init msm7x27a_calculate_reserve_sizes(void)
{
	size_pmem_devices();
	reserve_pmem_memory();
}

static int msm7x27a_paddr_to_memtype(unsigned int paddr)
{
	return MEMTYPE_EBI1;
}

static struct reserve_info msm7x27a_reserve_info __initdata = {
	.memtype_reserve_table = msm7x27a_reserve_table,
	.calculate_reserve_sizes = msm7x27a_calculate_reserve_sizes,
	.paddr_to_memtype = msm7x27a_paddr_to_memtype,
};

static void __init msm7x27a_reserve(void)
{
	reserve_info = &msm7x27a_reserve_info;
	msm_reserve();
}
#endif

static void __init msm_device_i2c_init(void)
{
	msm_gsbi0_qup_i2c_device.dev.platform_data = &msm_gsbi0_qup_i2c_pdata;
	msm_gsbi1_qup_i2c_device.dev.platform_data = &msm_gsbi1_qup_i2c_pdata;
}

static int msm7x27a_ts_cy8c_power(int on)
{
	printk(KERN_INFO "%s():\n", __func__);

	if (on) {
		gpio_set_value(PICO_V_TP_3V3_EN, 1);
		msleep(2);
		gpio_set_value(PICO_GPIO_TP_RST_N, 0);
		msleep(2);
		gpio_set_value(PICO_GPIO_TP_RST_N, 1);
	}
	return 0;
}

static int msm7x27a_ts_cy8c_wake(void)
{
	printk(KERN_INFO "%s():\n", __func__);
	gpio_direction_output(PICO_GPIO_TP_ATT_N, 0);
	udelay(500);
	gpio_direction_input(PICO_GPIO_TP_ATT_N);

	return 0;
}

/*static int msm7x27a_ts_cy8c_reset(void)
{
	printk(KERN_INFO "%s():\n", __func__);
	gpio_direction_output(PICO_GPIO_TP_RST_N,0);
	mdelay(100);
	gpio_direction_output(PICO_GPIO_TP_RST_N,1);
	return 0;
}*/

struct cy8c_i2c_platform_data msm7x27a_ts_cy8c_data[] = {
	{
		.version = 0x00,
		.abs_x_min = 6,
		.abs_x_max = 1015,
		.abs_y_min = 0,
		.abs_y_max = 906,
		.abs_pressure_min = 0,
		.abs_pressure_max = 255,
		.abs_width_min = 0,
		.abs_width_max = 512,
		.event_htc_enable = 0,
		.power = msm7x27a_ts_cy8c_power,
		.wake = msm7x27a_ts_cy8c_wake,
		.gpio_irq = PICO_GPIO_TP_ATT_N,
	},
};

static int msm7x27a_ts_himax_power(int on)
{
	printk(KERN_INFO "%s():\n", __func__);
	if (on)
		gpio_set_value(PICO_V_TP_3V3_EN, 1);

	return 0;
}

static void msm7x27a_ts_himax_reset(void)
{
	printk(KERN_INFO "%s():\n", __func__);
	gpio_direction_output(PICO_GPIO_TP_RST_N, 0);
	mdelay(10);
	gpio_direction_output(PICO_GPIO_TP_RST_N, 1);
}

static int msm7x27a_ts_himax_loadSensorConfig(struct i2c_client *client, struct himax_i2c_platform_data **pdata, struct himax_config_init_api *i2c_api);

struct himax_i2c_platform_data msm7x27a_ts_himax_data[] = {
	{
		.slave_addr = 0x90,
		.version = 0x00,
		.abs_x_min = 0,
		.abs_x_max = 1024,
		.abs_y_min = 0,
		.abs_y_max = 910,
		.abs_pressure_min = 0,
		.abs_pressure_max = 256,
		.abs_width_min = 0,
		.abs_width_max = 256,
		.gpio_irq = PICO_GPIO_TP_ATT_N,
		.tw_id = 0,/* YFO */
		.event_htc_enable = 0,
		.cable_config = { 0x90, 0x00},
		.power = msm7x27a_ts_himax_power,
		.loadSensorConfig = msm7x27a_ts_himax_loadSensorConfig,
		.reset = msm7x27a_ts_himax_reset,
		.c1 = { 0x36, 0x0F, 0x53 },
		.c2 = { 0xDD, 0x04, 0x02 },
		.c3 = { 0x37, 0xFF, 0x08, 0xFF, 0x08 },
		.c4 = { 0x39, 0x03 },
		.c5 = { 0x3A, 0x00 },
		.c6 = { 0x6E, 0x04 },
		.c7 = { 0x76, 0x01, 0x36 },
		.c8 = { 0x78, 0x03 },
		.c9 = { 0x7A, 0x00, 0xD8, 0x0C },
		.c10 = { 0x7D, 0x00, 0x04, 0x0A, 0x0A, 0x04 },
		.c11 = { 0x7F, 0x05, 0x01, 0x01, 0x01, 0x01, 0x07, 0x0D, 0x0B, 0x0D, 0x02,
			 0x0B, 0x02, 0x0B, 0x00 },
		.c12 = { 0xC2, 0x11, 0x00, 0x00, 0x00 },
		.c13 = { 0xC5, 0x0A, 0x1D, 0x00, 0x10, 0x1B, 0x1F, 0x0B },
		.c14 = { 0xC6, 0x11, 0x10, 0x16 },
		.c15 = { 0xCB, 0x01, 0xF5, 0xFF, 0xFF, 0x01, 0x00, 0x05, 0x00, 0x05, 0x00 },
		.c16 = { 0xD4, 0x01, 0x04, 0x07 },
		.c17 = { 0xD5, 0x55 },
		.c18 = { 0x62, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x00 },
		.c19 = { 0x63, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00 },
		.c20 = { 0x64, 0x13, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00 },
		.c21 = { 0x65, 0x00, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.c22 = { 0x66, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00 },
		.c23 = { 0x67, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.c24 = { 0x68, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x00 },
		.c25 = { 0x69, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x00 },
		.c26 = { 0x6A, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x00 },
		.c27 = { 0x6B, 0x40, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00 },
		.c28 = { 0x6C, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.c29 = { 0x6D, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00 },
		.c30 = { 0xC9, 0x00, 0x00, 0x00, 0x00, 0x2E, 0x2E, 0x2C, 0x2C, 0x26, 0x26,
			 0x25, 0x25, 0x06, 0x06, 0x1C, 0x1C, 0x2D, 0x2D, 0x2B, 0x2B, 0x22,
			 0x22 },
		.c31 = { 0x8A, 0xFF, 0xFF, 0x02, 0x09, 0xFF, 0x04, 0xFF, 0x07, 0x0D, 0xFF,
			 0x0C, 0x0F, 0xFF, 0xFF, 0x14, 0x15, 0x08, 0xFF, 0xFF, 0x10, 0x13,
			 0xFF, 0xFF, 0x16, 0x0E, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0x00, 0x03,
			 0xFF, 0xFF, 0x0A, 0x01, 0xFF, 0xFF, 0x17, 0x0B, 0xFF, 0xFF, 0xFF,
			 0x11, 0xFF, 0x06, 0xFF, 0x12 },
		.c32 = { 0x8C, 0x30, 0x0C, 0x0A, 0x0C, 0x08, 0x08, 0x08, 0x32, 0x24 },
		.c33 = { 0xE9, 0x00 },
		.c34 = { 0xEA, 0x0F, 0x09, 0x00, 0x24 },
		.c35 = { 0xEB, 0x2C, 0x22, 0x07, 0x83 },
		.c36 = { 0xEC, 0x00, 0x0F, 0x0A, 0x2D, 0x2D, 0x00, 0x00 },
		/*.c37 = { 0xEF, 0x11, 0x22 },*/
		.c38 = { 0xF0, 0x40 },
		/*.c39 = { 0xF1, 0x02, 0x04, 0x06, 0x06 },*/
		.c40 = { 0xF2, 0x0A, 0x3C, 0x14, 0x3C },
		.c41 = { 0xF3, 0x0F },
		.c42 = { 0xF4, 0x7D, 0x96, 0x1C, 0xC8 },
		.c43 = { 0xF6, 0x00, 0x00, 0x14, 0x2A, 0x05 },
		.c44 = { 0xF7, 0x05, 0x04, 0x00, 0x04, 0x00 },
		/* Added for Check Sum function 20110706 */
		.c45 = { 0xAB, 0x01 },
		.c46 = { 0xAB, 0x10 },
		/* Added for Check Sum Golden Pattern */
		.c47 = { 0xAC, 0xA8, 0x49 },
		/* Reset Check Sum funciton */
		.c48 = { 0xAB, 0x00 },
		.enterLeave = { 0xEF, 0x11, 0x22 },
		.averageDistance = { 0xF1, 0x02, 0x04, 0x06, 0x06 },
	},
	{
		.slave_addr = 0x90,
		.version = 0x00,
		.abs_x_min = 0,
		.abs_x_max = 1024,
		.abs_y_min = 0,
		.abs_y_max = 910,
		.abs_pressure_min = 0,
		.abs_pressure_max = 256,
		.abs_width_min = 0,
		.abs_width_max = 256,
		.gpio_irq = PICO_GPIO_TP_ATT_N,
		.tw_id = 1,/* Wintek */
		.event_htc_enable = 0,
		.cable_config = { 0x90, 0x00},
		.power = msm7x27a_ts_himax_power,
		.loadSensorConfig = msm7x27a_ts_himax_loadSensorConfig,
		.reset = msm7x27a_ts_himax_reset,
		.c1 = { 0x36, 0x0F, 0x53 },
		.c2 = { 0xDD, 0x04, 0x02 },
		.c3 = { 0x37, 0xFF, 0x08, 0xFF, 0x08 },
		.c4 = { 0x39, 0x03 },
		.c5 = { 0x3A, 0x00 },
		.c6 = { 0x6E, 0x04 },
		.c7 = { 0x76, 0x01, 0x36 },
		.c8 = { 0x78, 0x03 },
		.c9 = { 0x7A, 0x00, 0xD8, 0x0C },
		.c10 = { 0x7D, 0x00, 0x04, 0x0A, 0x0A, 0x04 },
		.c11 = { 0x7F, 0x05, 0x01, 0x01, 0x01, 0x01, 0x07, 0x0D, 0x0B, 0x0D, 0x02,
			 0x0B, 0x02, 0x0B, 0x00 },
		.c12 = { 0xC2, 0x11, 0x00, 0x00, 0x00 },
		.c13 = { 0xC5, 0x0A, 0x1D, 0x00, 0x10, 0x1D, 0x1F, 0x0B },
		.c14 = { 0xC6, 0x11, 0x10, 0x18 },
		.c15 = { 0xCB, 0x01, 0xF5, 0xFF, 0xFF, 0x01, 0x00, 0x05, 0x00, 0x05, 0x00 },
		.c16 = { 0xD4, 0x01, 0x04, 0x07 },
		.c17 = { 0xD5, 0x55 },
		.c18 = { 0x62, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x00 },
		.c19 = { 0x63, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00 },
		.c20 = { 0x64, 0x13, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00 },
		.c21 = { 0x65, 0x00, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.c22 = { 0x66, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00 },
		.c23 = { 0x67, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.c24 = { 0x68, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x00 },
		.c25 = { 0x69, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x00 },
		.c26 = { 0x6A, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x00 },
		.c27 = { 0x6B, 0x40, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00 },
		.c28 = { 0x6C, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.c29 = { 0x6D, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00 },
		.c30 = { 0xC9, 0x00, 0x00, 0x00, 0x00, 0x2E, 0x2E, 0x2C, 0x2C, 0x26, 0x26,
			 0x25, 0x25, 0x06, 0x06, 0x1C, 0x1C, 0x2D, 0x2D, 0x2B, 0x2B, 0x22,
			 0x22 },
		.c31 = { 0x8A, 0xFF, 0xFF, 0x02, 0x09, 0xFF, 0x04, 0xFF, 0x07, 0x0D, 0xFF,
			 0x0C, 0x0F, 0xFF, 0xFF, 0x14, 0x15, 0x08, 0xFF, 0xFF, 0x10, 0x13,
			 0xFF, 0xFF, 0x16, 0x0E, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0x00, 0x03,
			 0xFF, 0xFF, 0x0A, 0x01, 0xFF, 0xFF, 0x17, 0x0B, 0xFF, 0xFF, 0xFF,
			 0x11, 0xFF, 0x06, 0xFF, 0x12 },
		.c32 = { 0x8C, 0x30, 0x0C, 0x0A, 0x0C, 0x08, 0x08, 0x08, 0x32, 0x24 },
		.c33 = { 0xE9, 0x00 },
		.c34 = { 0xEA, 0x0F, 0x09, 0x00, 0x24 },
		.c35 = { 0xEB, 0x2C, 0x22, 0x07, 0x83 },
		.c36 = { 0xEC, 0x00, 0x0F, 0x0A, 0x2D, 0x2D, 0x00, 0x00 },
		/*.c37 = { 0xEF, 0x11, 0x22 },*/
		.c38 = { 0xF0, 0x40 },
		/*.c39 = { 0xF1, 0x02, 0x04, 0x06, 0x06 },*/
		.c40 = { 0xF2, 0x0A, 0x3C, 0x14, 0x3C },
		.c41 = { 0xF3, 0x0F },
		.c42 = { 0xF4, 0x7D, 0x96, 0x3E, 0xC8 },
		.c43 = { 0xF6, 0x00, 0x00, 0x14, 0x2A, 0x05 },
		.c44 = { 0xF7, 0x05, 0x04, 0x00, 0x04, 0x00 },
		/* Added for Check Sum function 20110706 */
		.c45 = { 0xAB, 0x01 },
		.c46 = { 0xAB, 0x10 },
		/* Added for Check Sum Golden Pattern */
		.c47 = { 0xAC, 0xCE, 0x49 },
		/* Reset Check Sum funciton */
		.c48 = { 0xAB, 0x00 },
		.enterLeave = { 0xEF, 0x11, 0x22 },
		.averageDistance = { 0xF1, 0x02, 0x04, 0x06, 0x06 },
	},
	{
		.slave_addr = 0x90,
		.version = 0x00,
		.abs_x_min = 0,
		.abs_x_max = 1024,
		.abs_y_min = 0,
		.abs_y_max = 910,
		.abs_pressure_min = 0,
		.abs_pressure_max = 256,
		.abs_width_min = 0,
		.abs_width_max = 256,
		.gpio_irq = PICO_GPIO_TP_ATT_N,
		.tw_id = 3,/* Cando */
		.event_htc_enable = 0,
		.cable_config = { 0x90, 0x00},
		.power = msm7x27a_ts_himax_power,
		.loadSensorConfig = msm7x27a_ts_himax_loadSensorConfig,
		.reset = msm7x27a_ts_himax_reset,
		.c1 = { 0x36, 0x0F, 0x53 },
		.c2 = { 0xDD, 0x04, 0x02 },
		.c3 = { 0x37, 0xFF, 0x08, 0xFF, 0x08 },
		.c4 = { 0x39, 0x03 },
		.c5 = { 0x3A, 0x00 },
		.c6 = { 0x6E, 0x04 },
		.c7 = { 0x76, 0x01, 0x36 },
		.c8 = { 0x78, 0x03 },
		.c9 = { 0x7A, 0x00, 0xD8, 0x0C },
		.c10 = { 0x7D, 0x00, 0x04, 0x0A, 0x0A, 0x04 },
		.c11 = { 0x7F, 0x05, 0x01, 0x01, 0x01, 0x01, 0x07, 0x0D, 0x0B, 0x0D, 0x02,
			 0x0B, 0x02, 0x0B, 0x00 },
		.c12 = { 0xC2, 0x11, 0x00, 0x00, 0x00 },
		.c13 = { 0xC5, 0x0A, 0x1D, 0x00, 0x10, 0x1D, 0x1F, 0x0B },
		.c14 = { 0xC6, 0x11, 0x10, 0x16 },
		.c15 = { 0xCB, 0x01, 0xF5, 0xFF, 0xFF, 0x01, 0x00, 0x05, 0x00, 0x05, 0x00 },
		.c16 = { 0xD4, 0x01, 0x04, 0x07 },
		.c17 = { 0xD5, 0x55 },
		.c18 = { 0x62, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x00 },
		.c19 = { 0x63, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00 },
		.c20 = { 0x64, 0x13, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00 },
		.c21 = { 0x65, 0x00, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.c22 = { 0x66, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00 },
		.c23 = { 0x67, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.c24 = { 0x68, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x00 },
		.c25 = { 0x69, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x00 },
		.c26 = { 0x6A, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x00 },
		.c27 = { 0x6B, 0x40, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00 },
		.c28 = { 0x6C, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.c29 = { 0x6D, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00 },
		.c30 = { 0xC9, 0x00, 0x00, 0x00, 0x00, 0x2E, 0x2E, 0x2C, 0x2C, 0x26, 0x26,
			 0x25, 0x25, 0x06, 0x06, 0x1C, 0x1C, 0x2D, 0x2D, 0x2B, 0x2B, 0x22,
			 0x22 },
		.c31 = { 0x8A, 0xFF, 0xFF, 0x02, 0x09, 0xFF, 0x04, 0xFF, 0x07, 0x0D, 0xFF,
			 0x0C, 0x0F, 0xFF, 0xFF, 0x14, 0x15, 0x08, 0xFF, 0xFF, 0x10, 0x13,
			 0xFF, 0xFF, 0x16, 0x0E, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0x00, 0x03,
			 0xFF, 0xFF, 0x0A, 0x01, 0xFF, 0xFF, 0x17, 0x0B, 0xFF, 0xFF, 0xFF,
			 0x11, 0xFF, 0x06, 0xFF, 0x12 },
		.c32 = { 0x8C, 0x30, 0x0C, 0x0A, 0x0C, 0x08, 0x08, 0x08, 0x32, 0x24 },
		.c33 = { 0xE9, 0x00 },
		.c34 = { 0xEA, 0x0F, 0x09, 0x00, 0x24 },
		.c35 = { 0xEB, 0x2C, 0x22, 0x07, 0x83 },
		.c36 = { 0xEC, 0x00, 0x0F, 0x0A, 0x2D, 0x2D, 0x00, 0x00 },
		/*.c37 = { 0xEF, 0x11, 0x22 },*/
		.c38 = { 0xF0, 0x40 },
		/*.c39 = { 0xF1, 0x02, 0x04, 0x06, 0x06 },*/
		.c40 = { 0xF2, 0x0A, 0x3C, 0x14, 0x3C },
		.c41 = { 0xF3, 0x0F },
		.c42 = { 0xF4, 0x7D, 0x96, 0x3E, 0xC8 },
		.c43 = { 0xF6, 0x00, 0x00, 0x14, 0x2A, 0x05 },
		.c44 = { 0xF7, 0x05, 0x04, 0x00, 0x04, 0x00 },
		/* Added for Check Sum function 20110706 */
		.c45 = { 0xAB, 0x01 },
		.c46 = { 0xAB, 0x10 },
		/* Added for Check Sum Golden Pattern */
		.c47 = { 0xAC, 0xCC, 0x49 },
		/* Reset Check Sum funciton */
		.c48 = { 0xAB, 0x00 },
		.enterLeave = { 0xEF, 0x11, 0x22 },
		.averageDistance = { 0xF1, 0x02, 0x04, 0x06, 0x06 },
	},
};

static int msm7x27a_ts_himax_loadSensorConfig(struct i2c_client *client, struct himax_i2c_platform_data **pdata, struct himax_config_init_api *i2c_api)
{
	uint8_t act_len;
	int result;
	char Data[1] = {0};
	char cmd[3] = {0};
	int retryTimes = 0, i = 0;

start:
	if (!pdata || !client || !i2c_api) {
		printk(KERN_ERR "[TOUCH_ERR]%s: Necessary parameters are null!\n", __func__);
		return -1;
	}
	result = i2c_api->i2c_himax_write_command(client, 0x81);
	if (result < 0) {
		printk(KERN_INFO "No Himax chip inside\n");
		return -EIO;
	} else {
		hr_msleep(240);

		cmd[0] = 0x36;
		cmd[1] = 0x0F;
		cmd[2] = 0x53;
		i2c_api->i2c_himax_master_write(client, cmd , sizeof(cmd));
		cmd[0] = 0xDD;
		cmd[1] = 0x04;
		cmd[2] = 0x02;
		i2c_api->i2c_himax_master_write(client, cmd , sizeof(cmd));
		i2c_api->i2c_himax_write_command(client, 0x83);
		hr_msleep(50);
		i2c_api->i2c_himax_write_command(client, 0x82);
		i2c_api->i2c_himax_write_command(client, 0xF5);
		i2c_api->i2c_himax_read_command(client, 1, Data, &act_len);

		for (i = 0; i < sizeof(msm7x27a_ts_himax_data)/sizeof(struct himax_i2c_platform_data); ++i) {
			if (msm7x27a_ts_himax_data[i].tw_id == (Data[0] & 0x03)) {
				*pdata = msm7x27a_ts_himax_data + i;
				break;
			}
		}
		switch (Data[0] & 0x03) {
		case 0x00:/* YFO */
			printk(KERN_INFO "%s: YFO touch window detected.\n", __func__);
			break;
		case 0x01:/* Wintek */
			printk(KERN_INFO "%s: Wintek touch window detected.\n", __func__);
			break;
		case 0x03:/* Cando */
			printk(KERN_INFO "%s: Cando touch window detected.\n", __func__);
		}
		if (i == sizeof(msm7x27a_ts_himax_data)/sizeof(struct himax_i2c_platform_data)) {
			printk(KERN_ERR "[TOUCH_ERR]%s: Couldn't find the matched profile!\n", __func__);
			return -1;
		}

		printk(KERN_INFO "%s: start initializing Sensor configs\n", __func__);
	}

	do {
		if (retryTimes == 5) {
			msm7x27a_ts_himax_reset();
			hr_msleep(50);
			++retryTimes;
			goto start;
		} else if (retryTimes == 11) {
			printk(KERN_ERR "[TOUCH_ERR]%s: Himax configuration checksum error!\n", __func__);
			return -EIO;
		}

		/* Start Check Sum */
		i2c_api->i2c_himax_master_write(client, (*pdata)->c48 , sizeof((*pdata)->c48));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c45 , sizeof((*pdata)->c45));

		i2c_api->i2c_himax_master_write(client, (*pdata)->c1 , sizeof((*pdata)->c1));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c2 , sizeof((*pdata)->c2));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c3 , sizeof((*pdata)->c3));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c4 , sizeof((*pdata)->c4));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c5 , sizeof((*pdata)->c5));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c6 , sizeof((*pdata)->c6));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c7 , sizeof((*pdata)->c7));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c8 , sizeof((*pdata)->c8));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c9 , sizeof((*pdata)->c9));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c10, sizeof((*pdata)->c10));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c11, sizeof((*pdata)->c11));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c12, sizeof((*pdata)->c12));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c13, sizeof((*pdata)->c13));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c14, sizeof((*pdata)->c14));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c15, sizeof((*pdata)->c15));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c16, sizeof((*pdata)->c16));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c17, sizeof((*pdata)->c17));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c18, sizeof((*pdata)->c18));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c19, sizeof((*pdata)->c19));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c20, sizeof((*pdata)->c20));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c21, sizeof((*pdata)->c21));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c22, sizeof((*pdata)->c22));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c23, sizeof((*pdata)->c23));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c24, sizeof((*pdata)->c24));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c25, sizeof((*pdata)->c25));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c26, sizeof((*pdata)->c26));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c27, sizeof((*pdata)->c27));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c28, sizeof((*pdata)->c28));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c29, sizeof((*pdata)->c29));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c30, sizeof((*pdata)->c30));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c31, sizeof((*pdata)->c31));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c32, sizeof((*pdata)->c32));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c33, sizeof((*pdata)->c33));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c34, sizeof((*pdata)->c34));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c35, sizeof((*pdata)->c35));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c36, sizeof((*pdata)->c36));
		i2c_api->i2c_himax_master_write(client, (*pdata)->enterLeave, sizeof((*pdata)->enterLeave));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c38, sizeof((*pdata)->c38));
		i2c_api->i2c_himax_master_write(client, (*pdata)->averageDistance, sizeof((*pdata)->averageDistance));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c40, sizeof((*pdata)->c40));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c41, sizeof((*pdata)->c41));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c42, sizeof((*pdata)->c42));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c43, sizeof((*pdata)->c43));
		i2c_api->i2c_himax_master_write(client, (*pdata)->c44, sizeof((*pdata)->c44));

		/* Stop Check Sum */
		i2c_api->i2c_himax_master_write(client, (*pdata)->c46, sizeof((*pdata)->c46));

		/* Enter Golden Pattern */
		i2c_api->i2c_himax_master_write(client, (*pdata)->c47, sizeof((*pdata)->c47));

		/* Read Hardware Check Sum */
		i2c_api->i2c_himax_write_command(client, 0xAB);
		i2c_api->i2c_himax_read_command(client, 1, Data, &act_len);
		++retryTimes;
	/* Check Software and Hardware Check Sum */
	} while (Data[0] != 0x10);

	i2c_api->i2c_himax_write_command(client, 0x83);
	hr_msleep(100);

	return result;
}

static struct i2c_board_info i2c_touch_device[] = {
	{
		I2C_BOARD_INFO(CYPRESS_TMA_NAME, 0xCE >> 1),
		.platform_data = &msm7x27a_ts_cy8c_data,
		.irq = MSM_GPIO_TO_INT(PICO_GPIO_TP_ATT_N)
	},
	{
		I2C_BOARD_INFO(HIMAX8526A_NAME, 0x90 >> 1),
		.platform_data = &msm7x27a_ts_himax_data,
		.irq = MSM_GPIO_TO_INT(PICO_GPIO_TP_ATT_N)
	},
};

static struct i2c_board_info i2c_touch_pvt_device[] = {
	{
		I2C_BOARD_INFO(HIMAX8526A_NAME, 0x90 >> 1),
		.platform_data = &msm7x27a_ts_himax_data,
		.irq = MSM_GPIO_TO_INT(PICO_GPIO_TP_ATT_N)
	},
};

static ssize_t msm7x27a_virtual_keys_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
		__stringify(EV_KEY) ":" __stringify(KEY_HOME)	    ":15:528:60:65"
		":" __stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":107:528:62:65"
		":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":212:528:68:65"
		":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":303:528:62:65"
		"\n");
}

static struct kobj_attribute msm7x27a_himax_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.himax-touchscreen",
		.mode = S_IRUGO,
	},
	.show = &msm7x27a_virtual_keys_show,
};

static struct kobj_attribute msm7x27a_cy8c_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.cy8c-touchscreen",
		.mode = S_IRUGO,
	},
	.show = &msm7x27a_virtual_keys_show,
};

static struct attribute *msm7x27a_properties_attrs[] = {
	&msm7x27a_himax_virtual_keys_attr.attr,
	&msm7x27a_cy8c_virtual_keys_attr.attr,
	NULL
};

static struct attribute_group msm7x27a_properties_attr_group = {
	.attrs = msm7x27a_properties_attrs,
};

static void pico_reset(void)
{
	gpio_set_value(PICO_GPIO_PS_HOLD, 0);
}

static void __init pico_init(void)
{
	int rc = 0;
	struct kobject *properties_kobj;

	msm7x2x_misc_init();

	printk(KERN_INFO "pico_init() revision = 0x%x\n", system_rev);
	//printk(KERN_INFO "MSM_PMEM_MDP_BASE=0x%x MSM_PMEM_ADSP_BASE=0x%x MSM_RAM_CONSOLE_BASE=0x%x MSM_FB_BASE=0x%x\n",
	//	MSM_PMEM_MDP_BASE, MSM_PMEM_ADSP_BASE, MSM_RAM_CONSOLE_BASE, MSM_FB_BASE);
	/* Must set msm_hw_reset_hook before first proc comm */
	msm_hw_reset_hook = pico_reset;

#ifdef CONFIG_PERFLOCK_BOOT_LOCK
	perflock_init(&holiday_perflock_data);
#endif

	/* Common functions for SURF/FFA/RUMI3 */
	msm_device_i2c_init();

	rc = pico_init_mmc(system_rev);
	if (rc)
		printk(KERN_CRIT "%s: MMC init failure (%d)\n", __func__, rc);

#ifdef CONFIG_BT
	bt_export_bd_address();
#endif

#ifdef CONFIG_SERIAL_MSM_HS
	msm_uart_dm1_pdata.rx_wakeup_irq = gpio_to_irq(PICO_GPIO_BT_HOST_WAKE);
	msm_device_uart_dm1.name = "msm_serial_hs_brcm"; /* for brcm */
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
	msm_otg_pdata.swfi_latency =
		msm7x27a_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
#endif
	headset_init();
	platform_add_devices(msm_footswitch_devices,
		msm_num_footswitch_devices);
	platform_add_devices(pico_devices,
			ARRAY_SIZE(pico_devices));

	msm_pm_set_platform_data(msm7x27a_pm_data,
				ARRAY_SIZE(msm7x27a_pm_data));

	pico_init_panel();
#if defined(CONFIG_I2C) && defined(CONFIG_GPIO_SX150X)
	register_i2c_devices();
#endif
#if defined(CONFIG_BT) && defined(CONFIG_MARIMBA_CORE)
	bt_power_init();
#endif
	properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (properties_kobj)
		rc = sysfs_create_group(properties_kobj,
						&msm7x27a_properties_attr_group);
	if (!properties_kobj || rc)
		pr_err("failed to create board_properties\n");

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_tps65200_devices, ARRAY_SIZE(i2c_tps65200_devices));
#ifdef CONFIG_MSM_CAMERA
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
			i2c_camera_devices,
			ARRAY_SIZE(i2c_camera_devices));
#endif
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_bma250_devices, ARRAY_SIZE(i2c_bma250_devices));

	/* Disable loading because of no Cypress chip consider by pcbid */
	if (system_rev >= 0x80) {
		printk(KERN_INFO "No Cypress chip!\n");
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_touch_pvt_device, ARRAY_SIZE(i2c_touch_pvt_device));
	} else
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_touch_device, ARRAY_SIZE(i2c_touch_device));

	pl_sensor_init();
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID, i2c_CM3628_devices,
				ARRAY_SIZE(i2c_CM3628_devices));

	pico_init_keypad();
	pico_wifi_init();

#ifdef CONFIG_MSM_RPC_VIBRATOR
	msm_init_pmic_vibrator();
#endif
#ifdef CONFIG_USB_ANDROID
	pico_add_usb_devices();
#endif
#ifdef CONFIG_MSM_HTC_DEBUG_INFO
	htc_debug_info_init();
#endif
#if defined(CONFIG_MSM_SERIAL_DEBUGGER)
	if (!opt_disable_uart3)
		msm_serial_debug_init(MSM_UART3_PHYS, INT_UART3,
				&msm_device_uart3.dev, 1,
				MSM_GPIO_TO_INT(PICO_GPIO_UART3_RX));
#endif
	/*7x25a kgsl initializations*/
	msm7x25a_kgsl_3d0_init();
}

static void __init pico_fixup(struct machine_desc *desc, struct tag *tags,
                               char **cmdline, struct meminfo *mi)
{
	mi->nr_banks=1;
	mi->bank[0].start = MSM_LINUX_BASE;
	mi->bank[0].size = MSM_LINUX_SIZE;
}

MACHINE_START(PICO, "pico")
	.boot_params	= PHYS_OFFSET + 0x100,
	.fixup = pico_fixup,
	.map_io		= msm_common_io_init,
#ifdef CONFIG_MSM_RESERVE_PMEM
	.reserve	= msm7x27a_reserve,
#endif
	.init_irq	= msm_init_irq,
	.init_machine	= pico_init,
	.timer		= &msm_timer,
MACHINE_END
