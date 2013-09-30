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
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <mach/system.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#ifdef CONFIG_USB_MSM_OTG_72K
#include <mach/msm_hsusb.h>
#else
#include <linux/usb/msm_hsusb.h>
#endif
#include <mach/htc_usb.h>
#include <mach/rpc_pmapp.h>
#ifdef CONFIG_USB_G_ANDROID
#include <linux/usb/android_composite.h>
#include <mach/usbdiag.h>
#endif
#include <mach/msm_memtypes.h>
#include <mach/msm_serial_hs.h>
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
#include <linux/regulator/consumer.h>
#include <mach/rpc_pmapp.h>
#include <mach/msm_battery.h>
#include <mach/htc_headset_mgr.h>
#include <mach/htc_headset_gpio.h>
#include <mach/htc_headset_pmic.h>
#include <linux/smsc911x.h>
#include <linux/atmel_maxtouch.h>
#include <linux/synaptics_i2c_rmi.h>
#include <linux/himax8526a.h>
#include "devices.h"
#include "timer.h"
#include "board-msm7x27a-regulator.h"
#include "devices-msm7x2xa.h"
#include "pm.h"
#include "pm-boot.h"
#include <mach/rpc_server_handset.h>
#include <mach/socinfo.h>
#include <linux/bma250.h>
#include <mach/htc_battery.h>
#include <linux/tps65200.h>
#include <linux/cm3629.h>
#include <linux/cm3628.h>
#include "board-pico.h"
#include <mach/board_htc.h>
#include <linux/htc_touch.h>
#include <linux/proc_fs.h>
#include <linux/leds-pm8029.h>
#include <linux/msm_audio.h>

#ifdef CONFIG_CODEC_AIC3254
#include <linux/spi_aic3254.h>
#endif

#ifdef CONFIG_BT
#include <mach/htc_bdaddress.h>
#include <mach/htc_sleep_clk.h>
#endif
#include <mach/htc_util.h>

#include <mach/cable_detect.h>
int htc_get_usb_accessory_adc_level(uint32_t *buffer);

static int config_gpio_table(uint32_t *table, int len);

#define PMEM_KERNEL_EBI1_SIZE	0x3A000
#define MSM_PMEM_AUDIO_SIZE  0x1F4000 //0x5B000

enum {
	GPIO_EXPANDER_IRQ_BASE	= NR_MSM_IRQS + NR_GPIO_IRQS,
	GPIO_EXPANDER_GPIO_BASE	= NR_MSM_GPIOS,
	/* SURF expander */
	GPIO_CORE_EXPANDER_BASE	= GPIO_EXPANDER_GPIO_BASE,
	//GPIO_BT_SYS_REST_EN	= GPIO_CORE_EXPANDER_BASE,
	GPIO_WLAN_EXT_POR_N	= GPIO_CORE_EXPANDER_BASE + 1,
	GPIO_DISPLAY_PWR_EN,
	GPIO_BACKLIGHT_EN,
	GPIO_PRESSURE_XCLR,
	GPIO_VREG_S3_EXP,
	GPIO_UBM2M_PWRDWN,
	GPIO_ETM_MODE_CS_N,
	GPIO_HOST_VBUS_EN,
	GPIO_SPI_MOSI,
	GPIO_SPI_MISO,
	GPIO_SPI_CLK,
	GPIO_SPI_CS0_N,
	GPIO_CORE_EXPANDER_IO13,
	GPIO_CORE_EXPANDER_IO14,
	GPIO_CORE_EXPANDER_IO15,
	/* Camera expander */
	GPIO_CAM_EXPANDER_BASE	= GPIO_CORE_EXPANDER_BASE + 16,
	GPIO_CAM_GP_STROBE_READY	= GPIO_CAM_EXPANDER_BASE,
	GPIO_CAM_GP_AFBUSY,
	GPIO_CAM_GP_CAM_PWDN,
	GPIO_CAM_GP_CAM1MP_XCLR,
	GPIO_CAM_GP_CAMIF_RESET_N,
	GPIO_CAM_GP_STROBE_CE,
	GPIO_CAM_GP_LED_EN1,
	GPIO_CAM_GP_LED_EN2,
};

/*static uint opt_disable_uart3;*/

#if defined(CONFIG_GPIO_SX150X)
enum {
	SX150X_CORE,
	SX150X_CAM,
};

static struct sx150x_platform_data sx150x_data[] __initdata = {
	[SX150X_CORE]	= {
		.gpio_base		= GPIO_CORE_EXPANDER_BASE,
		.oscio_is_gpo		= false,
		.io_pullup_ena		= 0,
		.io_pulldn_ena		= 0x02,
		.io_open_drain_ena	= 0xfef8,
		.irq_summary		= -1,
	},
	[SX150X_CAM]	= {
		.gpio_base		= GPIO_CAM_EXPANDER_BASE,
		.oscio_is_gpo		= false,
		.io_pullup_ena		= 0,
		.io_pulldn_ena		= 0,
		.io_open_drain_ena	= 0x23,
		.irq_summary		= -1,
	},
};
#endif

#if defined(CONFIG_I2C) && defined(CONFIG_GPIO_SX150X)
static struct i2c_board_info core_exp_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("sx1509q", 0x3e),
	},
};
static struct i2c_board_info cam_exp_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("sx1508q", 0x22),
		.platform_data	= &sx150x_data[SX150X_CAM],
	},
};
#endif

/* HEADSET DRIVER BEGIN */


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
	.adc_mic		= 14894,
	.adc_remote		= {0, 2209, 2210, 7605, 7606, 12048},
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
		.adc_max = 62311,
		.adc_min = 46975,
	},
	{
		.type = HEADSET_BEATS,
		.adc_max = 46974,
		.adc_min = 32977,
	},
	{
		.type = HEADSET_BEATS_SOLO,
		.adc_max = 32976,
		.adc_min = 17356,
	},
	{
		.type = HEADSET_NO_MIC, /* HEADSET_INDICATOR */
		.adc_max = 17355,
		.adc_min = 9045,
	},
	{
		.type = HEADSET_NO_MIC,
		.adc_max = 9044,
		.adc_min = 0,
	},
};

static struct htc_headset_mgr_platform_data htc_headset_mgr_data = {
	.driver_flag		= DRIVER_HS_MGR_OLD_AJ,
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

/* HEADSET DRIVER END */

struct platform_device htc_drm = {
	.name = "htcdrm",
	.id = 0,
};

#if defined(CONFIG_I2C) && defined(CONFIG_GPIO_SX150X)
static void __init register_i2c_devices(void)
{

	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
				cam_exp_i2c_info,
				ARRAY_SIZE(cam_exp_i2c_info));

	if (machine_is_msm7x27a_surf() || machine_is_msm7625a_surf())
		sx150x_data[SX150X_CORE].io_open_drain_ena = 0xe0f0;

	core_exp_i2c_info[0].platform_data =
			&sx150x_data[SX150X_CORE];

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				core_exp_i2c_info,
				ARRAY_SIZE(core_exp_i2c_info));
}
#endif

static uint32_t qup_i2c_gpio_table_io[] = {
	GPIO_CFG(60, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(61, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(131, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(132, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	};


static uint32_t qup_i2c_gpio_table_hw[] = {
	GPIO_CFG(60, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(61, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(131, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(132, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	};

static void gsbi_qup_i2c_gpio_config(int adap_id, int config_type)
{
	if (adap_id < 0 || adap_id > 1)
		return;

	pr_info("%s: adap_id = %d, config_type = %d\n", __func__, adap_id, config_type);

	if (config_type){
		gpio_tlmm_config(qup_i2c_gpio_table_hw[adap_id*2], GPIO_CFG_ENABLE);
		gpio_tlmm_config(qup_i2c_gpio_table_hw[adap_id*2 + 1], GPIO_CFG_ENABLE);
	} else {
		gpio_tlmm_config(qup_i2c_gpio_table_io[adap_id*2], GPIO_CFG_ENABLE);
		gpio_tlmm_config(qup_i2c_gpio_table_io[adap_id*2 + 1], GPIO_CFG_ENABLE);
	}
}

static struct msm_i2c_platform_data msm_gsbi0_qup_i2c_pdata = {
	.clk_freq		= 100000,
	.msm_i2c_config_gpio	= gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi1_qup_i2c_pdata = {
	.clk_freq		= 100000,
	.msm_i2c_config_gpio	= gsbi_qup_i2c_gpio_config,
};

#ifdef CONFIG_USB_G_ANDROID
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
	.fserial_init_string	= "tty:modem,tty,tty:serial",
	.nluns			= 2,
	.usb_id_pin_gpio	= PICO_GPIO_USB_ID,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};
#endif
static uint32_t usb_ID_PIN_input_table[] = {
	GPIO_CFG(PICO_GPIO_USB_ID, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t usb_ID_PIN_output_table[] = {
	GPIO_CFG(PICO_GPIO_USB_ID, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

void config_pico_usb_id_gpios(bool output)
{
	if (output) {
		gpio_tlmm_config(usb_ID_PIN_output_table[0], GPIO_CFG_ENABLE);
		gpio_set_value(PICO_GPIO_USB_ID, 1);
		printk(KERN_INFO "%s %d output high\n", __func__, PICO_GPIO_USB_ID);
	} else {
		gpio_tlmm_config(usb_ID_PIN_input_table[0], GPIO_CFG_ENABLE);
		printk(KERN_INFO "%s %d input none pull\n", __func__, PICO_GPIO_USB_ID);
	}
}


#define PM8058ADC_16BIT(adc) ((adc * 1800) / 65535) /* vref=2.2v, 16-bits resolution */
int64_t pico_get_usbid_adc(void)
{
	uint32_t adc_value = 0xffffffff;
	/*TODO: pico doesn't support accessory*/
	htc_get_usb_accessory_adc_level(&adc_value);
	adc_value = PM8058ADC_16BIT(adc_value);
	return adc_value;
}

static struct cable_detect_platform_data cable_detect_pdata = {
	.detect_type 		= CABLE_TYPE_PMIC_ADC,
	.usb_id_pin_gpio 	= PICO_GPIO_USB_ID,
	.config_usb_id_gpios 	= config_pico_usb_id_gpios,
	.get_adc_cb		= pico_get_usbid_adc,
};

static struct platform_device cable_detect_device = {
	.name	= "cable_detect",
	.id	= -1,
	.dev	= {
		.platform_data = &cable_detect_pdata,
	},
};

static int pico_phy_init_seq[] =
{
	0x2C, 0x31,
	0x2A, 0x32,
	0x06, 0x36,
	0x1D, 0x0D,
	0x1D, 0x10,
	-1
};

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
#endif

static struct msm_otg_platform_data msm_otg_pdata = {
#ifdef CONFIG_EHCI_MSM
	.vbus_power		 = msm_hsusb_vbus_power,
#endif
	.rpc_connect		 = hsusb_rpc_connect,
	.pemp_level		 = PRE_EMPHASIS_WITH_20_PERCENT,
	.cdr_autoreset		 = CDR_AUTO_RESET_DISABLE,
	.drv_ampl		 = HS_DRV_AMPLITUDE_DEFAULT,
	.se1_gating		 = SE1_GATING_DISABLE,
};

static struct msm_hsusb_gadget_platform_data msm_gadget_pdata = {
	.phy_init_seq		= pico_phy_init_seq,
	.is_phy_status_timer_on = 1,
};

#if (defined(CONFIG_MMC_MSM_SDC1_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC2_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC3_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC4_SUPPORT))

static unsigned long vreg_sts, gpio_sts;

struct sdcc_gpio {
	struct msm_gpio *cfg_data;
	uint32_t size;
	struct msm_gpio *sleep_cfg_data;
};

/**
 * Due to insufficient drive strengths for SDC GPIO lines some old versioned
 * SD/MMC cards may cause data CRC errors. Hence, set optimal values
 * for SDC slots based on timing closure and marginality. SDC1 slot
 * require higher value since it should handle bad signal quality due
 * to size of T-flash adapters.
 */
static struct msm_gpio sdc1_cfg_data[] = {
	{GPIO_CFG(51, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA),
								"sdc1_dat_3"},
	{GPIO_CFG(52, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA),
								"sdc1_dat_2"},
	{GPIO_CFG(53, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA),
								"sdc1_dat_1"},
	{GPIO_CFG(54, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA),
								"sdc1_dat_0"},
	{GPIO_CFG(55, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA),
								"sdc1_cmd"},
	{GPIO_CFG(56, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
								"sdc1_clk"},
};

static struct msm_gpio sdc2_cfg_data[] = {
	{GPIO_CFG(62, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_10MA),
								"sdc2_clk"},
	{GPIO_CFG(63, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc2_cmd"},
	{GPIO_CFG(64, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc2_dat_3"},
	{GPIO_CFG(65, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc2_dat_2"},
	{GPIO_CFG(66, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc2_dat_1"},
	{GPIO_CFG(67, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc2_dat_0"},
};
#endif

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

static struct msm_gpio sdc1_sleep_cfg_data[] = {
	{GPIO_CFG(51, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc1_dat_3"},
	{GPIO_CFG(52, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc1_dat_2"},
	{GPIO_CFG(53, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc1_dat_1"},
	{GPIO_CFG(54, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc1_dat_0"},
	{GPIO_CFG(55, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc1_cmd"},
	{GPIO_CFG(56, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc1_clk"},
};

static struct msm_gpio sdc2_sleep_cfg_data[] = {
	{GPIO_CFG(62, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
								"sdc2_clk"},
	{GPIO_CFG(63, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
								"sdc2_cmd"},
	{GPIO_CFG(64, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
								"sdc2_dat_3"},
	{GPIO_CFG(65, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
								"sdc2_dat_2"},
	{GPIO_CFG(66, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
								"sdc2_dat_1"},
	{GPIO_CFG(67, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
								"sdc2_dat_0"},
};
static struct msm_gpio sdc3_cfg_data[] = {
	{GPIO_CFG(88, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
								"sdc3_clk"},
	{GPIO_CFG(89, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc3_cmd"},
	{GPIO_CFG(90, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc3_dat_3"},
	{GPIO_CFG(91, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc3_dat_2"},
	{GPIO_CFG(92, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc3_dat_1"},
	{GPIO_CFG(93, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc3_dat_0"},
#ifdef CONFIG_MMC_MSM_SDC3_8_BIT_SUPPORT
	{GPIO_CFG(19, 3, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc3_dat_7"},
	{GPIO_CFG(20, 3, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc3_dat_6"},
	{GPIO_CFG(21, 3, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc3_dat_5"},
	{GPIO_CFG(108, 3, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc3_dat_4"},
#endif
};

static struct msm_gpio sdc4_cfg_data[] = {
	{GPIO_CFG(19, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc4_dat_3"},
	{GPIO_CFG(20, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc4_dat_2"},
	{GPIO_CFG(21, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc4_dat_1"},
	{GPIO_CFG(107, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc4_cmd"},
	{GPIO_CFG(108, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc4_dat_0"},
	{GPIO_CFG(109, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
								"sdc4_clk"},
};

static struct sdcc_gpio sdcc_cfg_data[] = {
	{
		.cfg_data = sdc1_cfg_data,
		.size = ARRAY_SIZE(sdc1_cfg_data),
		.sleep_cfg_data = sdc1_sleep_cfg_data,
	},
	{
		.cfg_data = sdc2_cfg_data,
		.size = ARRAY_SIZE(sdc2_cfg_data),
		.sleep_cfg_data = sdc2_sleep_cfg_data,
	},
	{
		.cfg_data = sdc3_cfg_data,
		.size = ARRAY_SIZE(sdc3_cfg_data),
	},
	{
		.cfg_data = sdc4_cfg_data,
		.size = ARRAY_SIZE(sdc4_cfg_data),
	},
};

static struct regulator *sdcc_vreg_data[ARRAY_SIZE(sdcc_cfg_data)];

static int msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct sdcc_gpio *curr;

	curr = &sdcc_cfg_data[dev_id - 1];
	if (!(test_bit(dev_id, &gpio_sts)^enable))
		return rc;

	if (enable) {
		set_bit(dev_id, &gpio_sts);
		rc = msm_gpios_request_enable(curr->cfg_data, curr->size);
		if (rc)
			pr_err("%s: Failed to turn on GPIOs for slot %d\n",
					__func__,  dev_id);
	} else {
		clear_bit(dev_id, &gpio_sts);
		if (curr->sleep_cfg_data) {
			rc = msm_gpios_enable(curr->sleep_cfg_data, curr->size);
			msm_gpios_free(curr->sleep_cfg_data, curr->size);
			return rc;
		}
		msm_gpios_disable_free(curr->cfg_data, curr->size);
	}
	return rc;
}

static int msm_sdcc_setup_vreg(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct regulator *curr = sdcc_vreg_data[dev_id - 1];

	if (test_bit(dev_id, &vreg_sts) == enable)
		return 0;

	if (!curr)
		return -ENODEV;

	if (IS_ERR(curr))
		return PTR_ERR(curr);
	mdelay(5);
	if (enable) {
		if (dev_id == 1)
			printk(KERN_INFO "%s: Enabling SD slot power\n", __func__);
		set_bit(dev_id, &vreg_sts);

		rc = regulator_enable(curr);
		if (rc)
			pr_err("%s: could not enable regulator: %d\n",
					__func__, rc);
	} else {
		if (dev_id == 1)
			printk(KERN_INFO "%s: Disabling SD slot power\n", __func__);
		clear_bit(dev_id, &vreg_sts);

		rc = regulator_disable(curr);
		if (rc)
			pr_err("%s: could not disable regulator: %d\n",
					__func__, rc);
	}
	return rc;
}

static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);

	rc = msm_sdcc_setup_gpio(pdev->id, !!vdd);
	if (rc)
		goto out;

	rc = msm_sdcc_setup_vreg(pdev->id, !!vdd);
out:
	return rc;
}

#define GPIO_SDC1_HW_DET 38

#if defined(CONFIG_MMC_MSM_SDC1_SUPPORT) \
	&& defined(CONFIG_MMC_MSM_CARD_HW_DETECTION)
static unsigned int msm7x2xa_sdcc_slot_status(struct device *dev)
{
	int status;

	status = gpio_tlmm_config(GPIO_CFG(GPIO_SDC1_HW_DET, 2, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (status)
		pr_err("%s:Failed to configure tlmm for GPIO %d\n", __func__,
				GPIO_SDC1_HW_DET);

	status = gpio_request(GPIO_SDC1_HW_DET, "SD_HW_Detect");
	if (status) {
		pr_err("%s:Failed to request GPIO %d\n", __func__,
				GPIO_SDC1_HW_DET);
	} else {
		status = gpio_direction_input(GPIO_SDC1_HW_DET);
		if (!status)
			status = !gpio_get_value(GPIO_SDC1_HW_DET);
		gpio_free(GPIO_SDC1_HW_DET);
	}
	return status;
}
#endif

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
static unsigned int pico_sdslot_type = MMC_TYPE_SD;
static struct mmc_platform_data sdc1_plat_data = {
	.ocr_mask	= MMC_VDD_28_29,
	.translate_vdd  = msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 25000000,
	.msmsdcc_fmax	= 50000000,
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	.status      = msm7x2xa_sdcc_slot_status,
	.status_irq  = MSM_GPIO_TO_INT(GPIO_SDC1_HW_DET),
	.irq_flags   = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
#endif
	.slot_type	= &pico_sdslot_type,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static unsigned int pico_sdc2_slot_type = MMC_TYPE_SDIO_WIFI;
static struct mmc_platform_data sdc2_plat_data = {
	/*
	 * SDC2 supports only 1.8V, claim for 2.85V range is just
	 * for allowing buggy cards who advertise 2.8V even though
	 * they can operate at 1.8V supply.
	 */
	.ocr_mask	= MMC_VDD_28_29 | MMC_VDD_165_195,
	.translate_vdd  = msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_SDIO_SUPPORT
	.sdiowakeup_irq = MSM_GPIO_TO_INT(66),
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 25000000,
	.msmsdcc_fmax	= 50000000,
	.slot_type      = &pico_sdc2_slot_type,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static unsigned int pico_sdc3_slot_type = MMC_TYPE_MMC;
static struct mmc_platform_data sdc3_plat_data = {
	.ocr_mask	= MMC_VDD_28_29,
#ifdef CONFIG_MMC_MSM_SDC3_8_BIT_SUPPORT
	.mmc_bus_width  = MMC_CAP_8_BIT_DATA,
#else
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 25000000,
	.msmsdcc_fmax	= 50000000,
	.slot_type      = &pico_sdc3_slot_type,
	.nonremovable	= 1,
	.emmc_dma_ch    = 7,
};
#endif

#if (defined(CONFIG_MMC_MSM_SDC4_SUPPORT)\
		&& !defined(CONFIG_MMC_MSM_SDC3_8_BIT_SUPPORT))
static struct mmc_platform_data sdc4_plat_data = {
	.ocr_mask	= MMC_VDD_28_29,
	.translate_vdd  = msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
};
#endif

static int __init mmc_regulator_init(int sdcc_no, const char *supply, int uV)
{
	int rc;

	BUG_ON(sdcc_no < 1 || sdcc_no > 4);

	sdcc_no--;

	sdcc_vreg_data[sdcc_no] = regulator_get(NULL, supply);

	if (IS_ERR(sdcc_vreg_data[sdcc_no])) {
		rc = PTR_ERR(sdcc_vreg_data[sdcc_no]);
		pr_err("%s: could not get regulator \"%s\": %d\n",
				__func__, supply, rc);
		goto out;
	}

	rc = regulator_set_voltage(sdcc_vreg_data[sdcc_no], uV, uV);

	if (rc) {
		pr_err("%s: could not set voltage for \"%s\" to %d uV: %d\n",
				__func__, supply, uV, rc);
		goto reg_free;
	}

	return rc;

reg_free:
	regulator_put(sdcc_vreg_data[sdcc_no]);
out:
	sdcc_vreg_data[sdcc_no] = NULL;
	return rc;
}

static void __init msm7x27a_init_mmc(void)
{
	/* eMMC slot */
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	if (mmc_regulator_init(3, "emmc", 3000000))
		return;
	msm_add_sdcc(3, &sdc3_plat_data);
#endif
	/* Micro-SD slot */
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	if (mmc_regulator_init(1, "mmc", 2850000))
		return;
	msm_add_sdcc(1, &sdc1_plat_data);
#endif
	/* SDIO WLAN slot */
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	if (mmc_regulator_init(2, "mmc", 2850000))
		return;
	msm_add_sdcc(2, &sdc2_plat_data);
#endif
	/* Not Used */
#if (defined(CONFIG_MMC_MSM_SDC4_SUPPORT)\
		&& !defined(CONFIG_MMC_MSM_SDC3_8_BIT_SUPPORT))
	if (mmc_regulator_init(4, "mmc", 2850000))
		return;
	msm_add_sdcc(4, &sdc4_plat_data);
#endif
}

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

#ifdef CONFIG_CODEC_AIC3254
static int aic3254_lowlevel_init(void)
{
	return 0;
}

static struct i2c_board_info i2c_aic3254_devices[] = {
	{
		I2C_BOARD_INFO(AIC3254_I2C_NAME, \
				AIC3254_I2C_ADDR),
	},
};
#endif
static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};

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

#define SND(desc, num) { .name = #desc, .id = num }
static struct snd_endpoint snd_endpoints_list[] = {
	SND(HANDSET, 0),
	SND(SPEAKER, 1),
	SND(HEADSET, 2),
	SND(BT, 3),
	SND(CARKIT, 4),
	SND(TTY_FULL, 5),
	SND(TTY_HEADSET, 5),
	SND(TTY_VCO, 6),
	SND(TTY_HCO, 7),
	SND(NO_MIC_HEADSET, 8),
	SND(FM_HEADSET, 9),
	SND(HEADSET_AND_SPEAKER, 10),
	SND(STEREO_HEADSET_AND_SPEAKER, 10),
	SND(FM_SPEAKER, 11),
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
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|
			(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
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

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
};
static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

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

#ifdef CONFIG_MSM_CAMERA
static uint32_t camera_off_gpio_table[] = {
	/* HTC_START */
	/* sleep status */
	GPIO_CFG(PICO_GPIO_CAMERA_SDA,  1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(PICO_GPIO_CAMERA_SCL,   1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(PICO_GPIO_CAMERA_RESET,     0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(PICO_GPIO_CAMERA_STANDBY,   0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(PICO_GPIO_CAMERA_MCLK,      0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),
	/* HTC_END */
};

static uint32_t camera_on_gpio_table[] = {
	/* HTC_START */
	/* sleep status */
	GPIO_CFG(PICO_GPIO_CAMERA_SDA,  1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(PICO_GPIO_CAMERA_SCL,   1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(PICO_GPIO_CAMERA_RESET,     0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(PICO_GPIO_CAMERA_STANDBY,   0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(PICO_GPIO_CAMERA_MCLK,      1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),
	/* HTC_END */
};

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

	if (machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa())
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
	if (machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa())
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

static struct msm_camera_sensor_flash_data flash_mt9t013 = {
	.flash_type = MSM_CAMERA_FLASH_NONE,

};

static struct msm_camera_sensor_info msm_camera_sensor_mt9t013_data = {
	.sensor_name    = "mt9t013",
	.sensor_reset   = PICO_GPIO_CAMERA_RESET,
	.sensor_pwd     = PICO_GPIO_CAMERA_STANDBY,
	.mclk           = PICO_GPIO_CAMERA_MCLK,
	.pdata          = &msm_camera_device_data_rear,
	.flash_data     = &flash_mt9t013,
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
	&msm_device_uart_dm2,
	&msm_device_nand,
	&msm_gsbi0_qup_i2c_device,
	&msm_gsbi1_qup_i2c_device,
	&htc_battery_pdev,
	&msm_device_otg,
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
	&msm_device_snd,
	&msm_device_adspdec,
	&msm_batt_device,
	&htc_headset_mgr,
	&htc_drm,
	&msm_kgsl_3d0,
#ifdef CONFIG_MT9T013
	&msm_camera_sensor_mt9t013,
#endif
#ifdef CONFIG_BT
	&wifi_bt_slp_clk,
	&pico_rfkill,
	&msm_device_uart_dm1,
#endif
	&cable_detect_device,
	&pm8029_leds,
};

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

static void __init msm_device_i2c_init(void)
{
	msm_gsbi0_qup_i2c_device.dev.platform_data = &msm_gsbi0_qup_i2c_pdata;
	msm_gsbi1_qup_i2c_device.dev.platform_data = &msm_gsbi1_qup_i2c_pdata;
}

#if defined(CONFIG_TOUCHSCREEN_HIMAX_SH)
static struct virtkey_attr virtkeys[] = {
	{ KEY_HOME,    40, 820, 100, 80 },
	{ KEY_MENU,   160, 820, 100, 80 },
	{ KEY_BACK,   300, 820, 100, 80 },
	{ KEY_SEARCH, 420, 830,  80, 80 }
};

static int touch_power(int on)
{
#if 0
	if (on)
		gpio_set_value(PICO_V_TP_3V3_EN, 1);
#endif
	return 0;
}

static void touch_config_pins(int state)
{
	switch (state) {
	case TP_INIT:
		gpio_tlmm_config(GPIO_CFG(PICO_GPIO_TP_ATT_N, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(PICO_GPIO_TP_RST_N, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
		break;
	case TP_SUSPEND:
		break;
	case TP_RESUME:
		break;
	}
}

uint8_t c01[] = { 0x36, 0x0F, 0x53 };
uint8_t c02[] = { 0xDD, 0x04, 0x02 };
uint8_t c03[] = { 0x37, 0xFF, 0x08, 0xFF, 0x08 };
uint8_t c04[] = { 0x39, 0x03 };
uint8_t c05[] = { 0x3A, 0x00 };
uint8_t c06[] = { 0x6E, 0x04 };
uint8_t c07[] = { 0x76, 0x01, 0x3F };
uint8_t c08[] = { 0x78, 0x03 };
uint8_t c09[] = { 0x7A, 0x00, 0x18, 0x0D };
uint8_t c10[] = { 0x7D, 0x00, 0x04, 0x0A, 0x0A, 0x04 };
uint8_t c11[] = { 0x7F, 0x05, 0x01, 0x01, 0x01, 0x01, 0x07, 0x0D, 0x0B, 0x0D, 0x0B,
		  0x0D, 0x02, 0x0B, 0x00 };
uint8_t c12[] = { 0xC2, 0x11, 0x00, 0x00, 0x00 };
uint8_t c13[] = { 0xC5, 0x0A, 0x1C, 0x00, 0x10, 0x18, 0x1F, 0x0B };
uint8_t c14[] = { 0xC6, 0x11, 0x10, 0x16 };
uint8_t c15[] = { 0xCB, 0x01, 0xF5, 0xFF, 0xFF, 0x01, 0x00, 0x05, 0x00, 0x05, 0x00 };
uint8_t c16[] = { 0xD4, 0x01, 0x04, 0x07 };
uint8_t c17[] = { 0xD5, 0xA5 };
uint8_t c18[] = { 0x62, 0x01, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00 };
uint8_t c19[] = { 0x63, 0x10, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00 };
uint8_t c20[] = { 0x64, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00 };
uint8_t c21[] = { 0x65, 0x10, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00 };
uint8_t c22[] = { 0x66, 0x41, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x00 };
uint8_t c23[] = { 0x67, 0x34, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x00 };
uint8_t c24[] = { 0x68, 0x40, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00 };
uint8_t c25[] = { 0x69, 0x34, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x00 };
uint8_t c26[] = { 0x6A, 0x43, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x00 };
uint8_t c27[] = { 0x6B, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00 };
uint8_t c28[] = { 0x6C, 0x41, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x00 };
uint8_t c29[] = { 0x6D, 0x24, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00 };
uint8_t c30[] = { 0xC9, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x0E, 0x0E, 0x10, 0x10,
		  0x11, 0x11, 0x13, 0x13, 0x15, 0x15, 0x17, 0x17, 0x18, 0x18, 0x1B,
		  0x1B, 0x1D, 0x1D, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t c31[] = { 0x8A, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38,
		  0xE4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x00, 0x00, 0x00,
		  0x1A, 0xFF, 0xFF, 0x01, 0x19, 0xFF, 0xFF, 0x02, 0xFF, 0x1B, 0xFF,
		  0x03, 0x18, 0xFF, 0xFF, 0x04, 0xFF, 0x1C, 0x0F, 0xFF, 0x17, 0x11,
		  0x0E, 0xFF, 0xFF, 0x1D, 0x0D, 0xFF, 0x16, 0x10, 0x0C, 0xFF, 0x15,
		  0x12, 0x0B, 0x05, 0xFF, 0xFF, 0x0A, 0x06, 0x14, 0xFF, 0x09, 0x13,
		  0x07, 0xFF, 0x08 };
uint8_t c32[] = { 0x8C, 0x30, 0x0C, 0x0A, 0x0C, 0x08, 0x08, 0x08, 0x32, 0x24, 0x40 };
uint8_t c33[] = { 0xE9, 0x00 };
uint8_t c34[] = { 0xEA, 0x13, 0x0B, 0x00, 0x24 };
uint8_t c35[] = { 0xEB, 0x28, 0x32, 0x8A, 0x83 };
uint8_t c36[] = { 0xEC, 0x00, 0x0F, 0x0A, 0x2D, 0x2D, 0x00, 0x00 };
uint8_t c37[] = { 0xEF, 0x11, 0x00 };
uint8_t c38[] = { 0xF0, 0x40 };
uint8_t c39[] = { 0xF1, 0x06, 0x04, 0x06, 0x03 };
uint8_t c40[] = { 0xF2, 0x0A, 0x06, 0x14, 0x3C };
uint8_t c41[] = { 0xF3, 0x07 };
uint8_t c42[] = { 0xF4, 0x7D, 0x96, 0x1E, 0xC8 };
uint8_t c43[] = { 0xF6, 0x00, 0x00, 0x14, 0x2A, 0x05 };
uint8_t c44[] = { 0xF7, 0x20, 0x4E, 0x00, 0x00, 0x00 };
uint8_t c45[] = { 0xED, 0x03, 0x06 };

static struct himax_cmd_size kim_cmds[] = {
	CS(c01), CS(c02), CS(c03), CS(c04), CS(c05), CS(c06),
	CS(c07), CS(c08), CS(c09), CS(c10), CS(c11), CS(c12),
	CS(c13), CS(c14), CS(c15), CS(c16), CS(c17), CS(c18),
	CS(c19), CS(c20), CS(c21), CS(c22), CS(c23), CS(c24),
	CS(c25), CS(c26), CS(c27), CS(c28), CS(c29), CS(c30),
	CS(c31), CS(c32), CS(c33), CS(c34), CS(c35), CS(c36),
	CS(c37), CS(c38), CS(c39), CS(c40), CS(c41), CS(c42),
	CS(c43), CS(c44), CS(c45)
};

static struct touch_devconfig himax_devconfigs[] = {
	TOUCH_DEVCONFIG(0xff, "KIM", kim_cmds),
};

static struct touch_platform_data hx8526a_pdata = {
	.abs_x_min	= 0,
	.abs_x_max	= 1023,
	.abs_y_min	= 0,
	.abs_y_max	= 910,
	.abs_p_min	= 0,
	.abs_p_max	= 255,
	.abs_w_min	= 0,
	.abs_w_max	= 255,
	.abs_z_min	= 0,
	.abs_z_max	= 255,
	.power		= touch_power,
	.virtkeys	= virtkeys,
	.virtkeys_nr    = ARRAY_SIZE(virtkeys),
	.vendor		= "himax",
	.gpio_irq	= PICO_GPIO_TP_ATT_N,
	.gpio_reset	= PICO_GPIO_TP_RST_N,
	.pin_config	= touch_config_pins,
	.devcfg_array	= himax_devconfigs,
	.devcfg_size	= ARRAY_SIZE(himax_devconfigs),
	.i2c_retries	= 3,
	.log_level	= 0,
	.irq_wakeup	= 1,
	.polling_mode	= 0,
	.trigger_low	= 1,
};
#endif

static int pico_ts_himax_power(int on)
{
	printk(KERN_INFO "%s():\n", __func__);
	if (on)
		gpio_set_value(PICO_V_TP_3V3_EN, 1);

	return 0;
}

static void pico_ts_himax_reset(void)
{
	printk(KERN_INFO "%s():\n", __func__);
	gpio_direction_output(PICO_GPIO_TP_RST_N, 0);
	mdelay(10);
	gpio_direction_output(PICO_GPIO_TP_RST_N, 1);
}

static int pico_ts_himax_loadSensorConfig(struct i2c_client *client, struct himax_i2c_platform_data **pdata, struct himax_config_init_api *i2c_api);

struct himax_i2c_platform_data pico_ts_himax_data[] = {
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
		.power = pico_ts_himax_power,
		.loadSensorConfig = pico_ts_himax_loadSensorConfig,
		.reset = pico_ts_himax_reset,
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
		.power = pico_ts_himax_power,
		.loadSensorConfig = pico_ts_himax_loadSensorConfig,
		.reset = pico_ts_himax_reset,
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
		.power = pico_ts_himax_power,
		.loadSensorConfig = pico_ts_himax_loadSensorConfig,
		.reset = pico_ts_himax_reset,
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

static int pico_ts_himax_loadSensorConfig(struct i2c_client *client, struct himax_i2c_platform_data **pdata, struct himax_config_init_api *i2c_api)
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

		for (i = 0; i < sizeof(pico_ts_himax_data)/sizeof(struct himax_i2c_platform_data); ++i) {
			if (pico_ts_himax_data[i].tw_id == (Data[0] & 0x03)) {
				*pdata = pico_ts_himax_data + i;
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
		if (i == sizeof(pico_ts_himax_data)/sizeof(struct himax_i2c_platform_data)) {
			printk(KERN_ERR "[TOUCH_ERR]%s: Couldn't find the matched profile!\n", __func__);
			return -1;
		}

		printk(KERN_INFO "%s: start initializing Sensor configs\n", __func__);
	}

	do {
		if (retryTimes == 5) {
			pico_ts_himax_reset();
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
		I2C_BOARD_INFO(HIMAX8526A_NAME, 0x90 >> 1),
		.platform_data = &pico_ts_himax_data,
		.irq = MSM_GPIO_TO_INT(PICO_GPIO_TP_ATT_N)
	},
};

static struct i2c_board_info i2c_touch_pvt_device[] = {
	{
		I2C_BOARD_INFO(HIMAX8526A_NAME, 0x90 >> 1),
		.platform_data = &pico_ts_himax_data,
		.irq = MSM_GPIO_TO_INT(PICO_GPIO_TP_ATT_N)
	},
};

static ssize_t pico_virtual_keys_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
		__stringify(EV_KEY) ":" __stringify(KEY_HOME)	    ":15:528:60:65"
		":" __stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":107:528:62:65"
		":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":212:528:68:65"
		":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":303:528:62:65"
		"\n");
}

static struct kobj_attribute pico_himax_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.himax-touchscreen",
		.mode = S_IRUGO,
	},
	.show = &pico_virtual_keys_show,
};

static struct attribute *pico_properties_attrs[] = {
	&pico_himax_virtual_keys_attr.attr,
	NULL
};

static struct attribute_group pico_properties_attr_group = {
	.attrs = pico_properties_attrs,
};

#define MSM_EBI2_PHYS			0xa0d00000
#define MSM_EBI2_XMEM_CS2_CFG1		0xa0d10030

static void __init msm7x27a_init_ebi2(void)
{
	uint32_t ebi2_cfg;
	void __iomem *ebi2_cfg_ptr;

	ebi2_cfg_ptr = ioremap_nocache(MSM_EBI2_PHYS, sizeof(uint32_t));
	if (!ebi2_cfg_ptr)
		return;

	ebi2_cfg = readl(ebi2_cfg_ptr);
	if (machine_is_msm7x27a_rumi3() || machine_is_msm7x27a_surf() ||
			machine_is_msm7625a_surf())
		ebi2_cfg |= (1 << 4); /* CS2 */

	writel(ebi2_cfg, ebi2_cfg_ptr);
	iounmap(ebi2_cfg_ptr);

	/* Enable A/D MUX[bit 31] from EBI2_XMEM_CS2_CFG1 */
	ebi2_cfg_ptr = ioremap_nocache(MSM_EBI2_XMEM_CS2_CFG1,
							 sizeof(uint32_t));
	if (!ebi2_cfg_ptr)
		return;

	ebi2_cfg = readl(ebi2_cfg_ptr);
	if (machine_is_msm7x27a_surf() || machine_is_msm7625a_surf())
		ebi2_cfg |= (1 << 31);

	writel(ebi2_cfg, ebi2_cfg_ptr);
	iounmap(ebi2_cfg_ptr);
}

#define ATMEL_TS_I2C_NAME "maXTouch"

static struct regulator_bulk_data regs_atmel[] = {
	{ .supply = "ldo2",  .min_uV = 2850000, .max_uV = 2850000 },
	{ .supply = "smps3", .min_uV = 1800000, .max_uV = 1800000 },
};

#define ATMEL_TS_GPIO_IRQ 82

static int atmel_ts_power_on(bool on)
{
	int rc = on ?
		regulator_bulk_enable(ARRAY_SIZE(regs_atmel), regs_atmel) :
		regulator_bulk_disable(ARRAY_SIZE(regs_atmel), regs_atmel);

	if (rc)
		pr_err("%s: could not %sable regulators: %d\n",
				__func__, on ? "en" : "dis", rc);
	else
		msleep(50);

	return rc;
}

static int atmel_ts_platform_init(struct i2c_client *client)
{
	int rc;
	struct device *dev = &client->dev;

	rc = regulator_bulk_get(dev, ARRAY_SIZE(regs_atmel), regs_atmel);
	if (rc) {
		dev_err(dev, "%s: could not get regulators: %d\n",
				__func__, rc);
		goto out;
	}

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs_atmel), regs_atmel);
	if (rc) {
		dev_err(dev, "%s: could not set voltages: %d\n",
				__func__, rc);
		goto reg_free;
	}

	rc = gpio_tlmm_config(GPIO_CFG(ATMEL_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (rc) {
		dev_err(dev, "%s: gpio_tlmm_config for %d failed\n",
			__func__, ATMEL_TS_GPIO_IRQ);
		goto reg_free;
	}

	/* configure touchscreen interrupt gpio */
	rc = gpio_request(ATMEL_TS_GPIO_IRQ, "atmel_maxtouch_gpio");
	if (rc) {
		dev_err(dev, "%s: unable to request gpio %d\n",
			__func__, ATMEL_TS_GPIO_IRQ);
		goto ts_gpio_tlmm_unconfig;
	}

	rc = gpio_direction_input(ATMEL_TS_GPIO_IRQ);
	if (rc < 0) {
		dev_err(dev, "%s: unable to set the direction of gpio %d\n",
			__func__, ATMEL_TS_GPIO_IRQ);
		goto free_ts_gpio;
	}
	return 0;

free_ts_gpio:
	gpio_free(ATMEL_TS_GPIO_IRQ);
ts_gpio_tlmm_unconfig:
	gpio_tlmm_config(GPIO_CFG(ATMEL_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);
reg_free:
	regulator_bulk_free(ARRAY_SIZE(regs_atmel), regs_atmel);
out:
	return rc;
}

static int atmel_ts_platform_exit(struct i2c_client *client)
{
	gpio_free(ATMEL_TS_GPIO_IRQ);
	gpio_tlmm_config(GPIO_CFG(ATMEL_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);
	regulator_bulk_disable(ARRAY_SIZE(regs_atmel), regs_atmel);
	regulator_bulk_free(ARRAY_SIZE(regs_atmel), regs_atmel);
	return 0;
}

static u8 atmel_ts_read_chg(void)
{
	return gpio_get_value(ATMEL_TS_GPIO_IRQ);
}

static u8 atmel_ts_valid_interrupt(void)
{
	return !atmel_ts_read_chg();
}

#define ATMEL_X_OFFSET 13
#define ATMEL_Y_OFFSET 0

static struct mxt_platform_data atmel_ts_pdata = {
	.numtouch = 4,
	.init_platform_hw = atmel_ts_platform_init,
	.exit_platform_hw = atmel_ts_platform_exit,
	.power_on = atmel_ts_power_on,
	.display_res_x = 480,
	.display_res_y = 864,
	.min_x = ATMEL_X_OFFSET,
	.max_x = (505 - ATMEL_X_OFFSET),
	.min_y = ATMEL_Y_OFFSET,
	.max_y = (863 - ATMEL_Y_OFFSET),
	.valid_interrupt = atmel_ts_valid_interrupt,
	.read_chg = atmel_ts_read_chg,
};

static struct i2c_board_info atmel_ts_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO(ATMEL_TS_I2C_NAME, 0x4a),
		.platform_data = &atmel_ts_pdata,
		.irq = MSM_GPIO_TO_INT(ATMEL_TS_GPIO_IRQ),
	},
};

static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
	.pwr_key_delay_ms = 500, /* 0 will disable end key */
};

static struct platform_device hs_pdev = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

static struct platform_device msm_proccomm_regulator_dev = {
	.name   = PROCCOMM_REGULATOR_DEV_NAME,
	.id     = -1,
	.dev    = {
		.platform_data = &msm7x27a_proccomm_regulator_data
	}
};

static void __init msm7x27a_init_regulators(void)
{
	int rc = platform_device_register(&msm_proccomm_regulator_dev);
	if (rc)
		pr_err("%s: could not register regulator device: %d\n",
				__func__, rc);
}

void pico_add_usb_devices(void)
{
	printk(KERN_INFO "%s rev: %d\n", __func__, system_rev);
	android_usb_pdata.products[0].product_id =
			android_usb_pdata.product_id;

	/* diag bit set */
	if (get_radio_flag() & 0x20000)
		android_usb_pdata.diag_init = 1;

	/* add cdrom support in normal mode */
	if (board_mfg_mode() == 0) {
		android_usb_pdata.nluns = 3;
		android_usb_pdata.cdrom_lun = 0x4;
	}

	msm_device_gadget_peripheral.dev.parent = &msm_device_otg.dev;
	platform_device_register(&msm_device_gadget_peripheral);
	platform_device_register(&android_usb_device);
}

static int __init board_serialno_setup(char *serialno)
{
	android_usb_pdata.serial_number = serialno;
	return 1;
}
__setup("androidboot.serialno=", board_serialno_setup);

static void pico_reset(void)
{
	gpio_set_value(PICO_GPIO_PS_HOLD, 0);
}

static void __init msm7x27a_otg_gadget(void)
{
	msm_device_otg.dev.platform_data = &msm_otg_pdata;
	msm_device_gadget_peripheral.dev.platform_data =
		&msm_gadget_pdata;
}

static void __init pico_init(void)
{
	int rc = 0, i = 0;
	struct kobject *properties_kobj;

	msm7x2x_misc_init();

	printk(KERN_INFO "pico_init() revision = 0x%x\n", system_rev);

	/* Must set msm_hw_reset_hook before first proc comm */
	msm_hw_reset_hook = pico_reset;

	/* Initialize regulators first so that other devices can use them */
	msm7x27a_init_regulators();

	/* Common functions for SURF/FFA/RUMI3 */
	msm_device_i2c_init();
	msm7x27a_init_ebi2();
	msm7x27a_otg_gadget();
	/* msm7x27a_cfg_uart2dm_serial(); */
#ifdef CONFIG_SERIAL_MSM_HS
	msm_uart_dm1_pdata.rx_wakeup_irq = gpio_to_irq(PICO_GPIO_BT_HOST_WAKE);
	msm_device_uart_dm1.name = "msm_serial_hs_brcm"; /* for brcm */
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
#endif

	if (board_mfg_mode() == 1)
		htc_headset_mgr_config[0].adc_max = 65535;

	/* msm7x27a_cfg_smsc911x(); */
	platform_add_devices(msm_footswitch_devices,
			msm_num_footswitch_devices);
	platform_add_devices(pico_devices,
			ARRAY_SIZE(pico_devices));

	/*Just init usb_id pin for accessory, accessory may not be used in pico */
	config_pico_usb_id_gpios(0);
	/*usb driver won't be loaded in MFG 58 station and gift mode*/
	if (!(board_mfg_mode() == 6 || board_mfg_mode() == 7))
		pico_add_usb_devices();

	pico_wifi_init();

	rc = pico_init_mmc(system_rev);
	if (rc)
		printk(KERN_CRIT "%s: MMC init failure (%d)\n", __func__, rc);

	msm_pm_set_platform_data(msm7x27a_pm_data,
				ARRAY_SIZE(msm7x27a_pm_data));
	BUG_ON(msm_pm_boot_init(MSM_PM_BOOT_CONFIG_RESET_VECTOR, ioremap(0x0, PAGE_SIZE)));
	pico_init_panel();
#if defined(CONFIG_I2C) && defined(CONFIG_GPIO_SX150X)
	register_i2c_devices();
#endif
#ifdef CONFIG_BT
	bt_export_bd_address();
#endif
	if (machine_is_msm7625a_surf() || machine_is_msm7625a_ffa()) {
		atmel_ts_pdata.min_x = 0;
		atmel_ts_pdata.max_x = 480;
		atmel_ts_pdata.min_y = 0;
		atmel_ts_pdata.max_y = 320;
	}

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_tps65200_devices, ARRAY_SIZE(i2c_tps65200_devices));

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
		atmel_ts_i2c_info,
		ARRAY_SIZE(atmel_ts_i2c_info));


	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_bma250_devices, ARRAY_SIZE(i2c_bma250_devices));
	pl_sensor_init();
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_CM3628_devices, ARRAY_SIZE(i2c_CM3628_devices));
#ifdef CONFIG_CODEC_AIC3254
	aic3254_lowlevel_init();
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_aic3254_devices, ARRAY_SIZE(i2c_aic3254_devices));
#endif

#ifdef CONFIG_MSM_CAMERA
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
			i2c_camera_devices,
			ARRAY_SIZE(i2c_camera_devices));
#endif
	platform_device_register(&hs_pdev);

	/*7x25a kgsl initializations*/
	msm7x25a_kgsl_3d0_init();

	/* Disable loading because of no Cypress chip consider by pcbid */
	if (system_rev >= 0x80) {
		printk(KERN_INFO "No Cypress chip!\n");
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_touch_pvt_device, ARRAY_SIZE(i2c_touch_pvt_device));
	} else

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				i2c_touch_device,
				ARRAY_SIZE(i2c_touch_device));

	properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (properties_kobj)
		rc = sysfs_create_group(properties_kobj,
				&pico_properties_attr_group);
	if (!properties_kobj || rc)
		pr_err("failed to create board_properties\n");


	pico_init_keypad();
#ifdef CONFIG_MSM_RPC_VIBRATOR
	msm_init_pmic_vibrator();
#endif

	if (get_kernel_flag() & KERNEL_FLAG_PM_MONITOR) {
		htc_monitor_init();
		htc_PM_monitor_init();
	}
}

static void __init pico_fixup(struct machine_desc *desc, struct tag *tags,
							char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 1;
	mi->bank[0].start = MSM_LINUX_BASE;
	mi->bank[0].size = MSM_LINUX_SIZE;
}

MACHINE_START(PICO, "pico")
	.boot_params	= PHYS_OFFSET + 0x100,
	.fixup = pico_fixup,
	.map_io		= msm_common_io_init,
	.reserve	= msm7x27a_reserve,
	.init_irq	= msm_init_irq,
	.init_machine	= pico_init,
	.timer		= &msm_timer,
MACHINE_END
