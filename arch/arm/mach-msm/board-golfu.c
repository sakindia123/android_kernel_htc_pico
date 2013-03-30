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
/* #include <linux/leds-pm8029.h> */
#include "board-golfu.h"
#include <mach/board_htc.h>
#include <linux/htc_touch.h>
#include <linux/htc_flashlight.h>
#include <linux/proc_fs.h>
#include <linux/pn544.h>

#ifdef CONFIG_CODEC_AIC3254
#include <linux/i2c/aic3254.h>
#endif

#ifdef CONFIG_BT
#include <mach/htc_bdaddress.h>
#endif
#include <mach/htc_util.h>
#ifdef CONFIG_PERFLOCK
#include <mach/perflock.h>
#endif

#include <mach/cable_detect.h>
int htc_get_usb_accessory_adc_level(uint32_t *buffer);

#define PMEM_KERNEL_EBI1_SIZE	0x3A000
#define MSM_PMEM_AUDIO_SIZE	0x5B000

extern int emmc_partition_read_proc(char *page, char **start, off_t off,
					int count, int *eof, void *data);

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


#ifdef CONFIG_PERFLOCK
static unsigned golfu_perf_acpu_table[] = {
       245760000,
       480000000,
       600000000,
};

static struct perflock_platform_data golfu_perflock_data = {
	.perf_acpu_table = golfu_perf_acpu_table,
	.table_size = ARRAY_SIZE(golfu_perf_acpu_table),
};
#endif
static struct platform_device msm_wlan_ar6000_pm_device = {
	.name           = "wlan_ar6000_pm_dev",
	.id             = -1,
};

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
	.hpin_gpio		= GOLFU_GPIO_AUD_HP_DET,
	.key_gpio		= GOLFU_GPIO_AUD_REMO_PRES,
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

#ifdef CONFIG_FLASHLIGHT_TPS61310
static void config_flashlight_gpios(void)
{
	static uint32_t flashlight_gpio_table[] = {
		GPIO_CFG(30, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG(26, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG(115, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	};

	gpio_tlmm_config(flashlight_gpio_table[0], GPIO_CFG_ENABLE);
	gpio_tlmm_config(flashlight_gpio_table[1], GPIO_CFG_ENABLE);
	gpio_tlmm_config(flashlight_gpio_table[2], GPIO_CFG_ENABLE);

	gpio_direction_output(30, 1);
	gpio_direction_output(26, 0);
	gpio_direction_output(115, 0);
}

static struct TPS61310_flashlight_platform_data tps61310_pdata = {
	.gpio_init = config_flashlight_gpios,
	.tps61310_strb0 = 26,
	.tps61310_strb1 = 115,
	/*.led_count = 1,*/
	.flash_duration_ms = 600,
};

static struct i2c_board_info tps61310_i2c_info[] = {
	{
		I2C_BOARD_INFO("TPS61310_FLASHLIGHT", 0x66 >> 1),
		.platform_data = &tps61310_pdata,
	},
};
#endif

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
/*
#ifdef CONFIG_ARCH_MSM7X27A
#define MSM_PMEM_MDP_SIZE       0x1900000
#define MSM_PMEM_ADSP_SIZE      0x1000000

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#define MSM_FB_SIZE		0x260000
#else
#define MSM_FB_SIZE		0x195000
#endif

#endif
*/
#ifdef CONFIG_USB_G_ANDROID
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id		= 0x0bb4,
	.product_id		= 0x0cf5,
	.version		= 0x0100,
	.product_name		= "Android Phone",
	.manufacturer_name	= "HTC",
	.num_products		= ARRAY_SIZE(usb_products),
	.products		= usb_products,
	.num_functions		= ARRAY_SIZE(usb_functions_all),
	.functions		= usb_functions_all,
	.fserial_init_string	= "tty:modem,tty,tty:serial",
	.nluns			= 2,
	.usb_id_pin_gpio	= GOLFU_GPIO_USB_ID,
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
	GPIO_CFG(GOLFU_GPIO_USB_ID, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t usb_ID_PIN_output_table[] = {
	GPIO_CFG(GOLFU_GPIO_USB_ID, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

void config_golfu_usb_id_gpios(bool output)
{
	if (output) {
		gpio_tlmm_config(usb_ID_PIN_output_table[0], GPIO_CFG_ENABLE);
		gpio_set_value(GOLFU_GPIO_USB_ID, 1);
		printk(KERN_INFO "%s %d output high\n", __func__, GOLFU_GPIO_USB_ID);
	} else {
		gpio_tlmm_config(usb_ID_PIN_input_table[0], GPIO_CFG_ENABLE);
		printk(KERN_INFO "%s %d input none pull\n", __func__, GOLFU_GPIO_USB_ID);
	}
}


#define PM8058ADC_16BIT(adc) ((adc * 1800) / 65535) /* vref=2.2v, 16-bits resolution */
int64_t golfu_get_usbid_adc(void)
{
	uint32_t adc_value = 0xffffffff;
	/*TODO: golfu doesn't support accessory*/
	htc_get_usb_accessory_adc_level(&adc_value);
	adc_value = PM8058ADC_16BIT(adc_value);
	return adc_value;
}

static struct cable_detect_platform_data cable_detect_pdata = {
	.detect_type 		= CABLE_TYPE_PMIC_ADC,
	.usb_id_pin_gpio 	= GOLFU_GPIO_USB_ID,
	.config_usb_id_gpios 	= config_golfu_usb_id_gpios,
	.get_adc_cb		= golfu_get_usbid_adc,
};

static struct platform_device cable_detect_device = {
	.name	= "cable_detect",
	.id	= -1,
	.dev	= {
		.platform_data = &cable_detect_pdata,
	},
};

static int golfu_phy_init_seq[] =
{
	0x2C, 0x31,
	0x2A, 0x32,
	0x06, 0x36,
	0x1D, 0x0D,
	0x1D, 0x10,
	-1
};

static struct msm_otg_platform_data msm_otg_pdata = {
	.phy_init_seq		= golfu_phy_init_seq,
	.mode			= USB_PERIPHERAL,
	.otg_control		= OTG_PMIC_CONTROL,
	.power_budget		= 750,
	.phy_type		= CI_45NM_INTEGRATED_PHY,
	.reset_phy_before_lpm	= 1,
};

#if 0
static struct resource smc91x_resources[] = {
	[0] = {
		.start = 0x90000300,
		.end   = 0x900003ff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = MSM_GPIO_TO_INT(4),
		.end   = MSM_GPIO_TO_INT(4),
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device smc91x_device = {
	.name           = "smc91x",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(smc91x_resources),
	.resource       = smc91x_resources,
};
#endif

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
	.gpio_chg_int = MSM_GPIO_TO_INT(GOLFU_GPIO_CHG_INT),
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
static unsigned int golfu_sdslot_type = MMC_TYPE_SD;
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
	.slot_type	= &golfu_sdslot_type,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static unsigned int golfu_sdc2_slot_type = MMC_TYPE_SDIO_WIFI;
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
	.slot_type      = &golfu_sdc2_slot_type,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static unsigned int golfu_sdc3_slot_type = MMC_TYPE_MMC;
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
	.slot_type      = &golfu_sdc3_slot_type,
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
	.inject_rx_on_wakeup	= 1,
	.rx_to_inject		= 0xFD,
};
#endif
/*
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
*/
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
	.intr = GOLFU_GPIO_GSENSORS_INT,
	.chip_layout = 0,
	.layouts = GOLFU_LAYOUTS,
};

static struct i2c_board_info i2c_bma250_devices[] = {
	{
		I2C_BOARD_INFO(BMA250_I2C_NAME_REMOVE_ECOMPASS, \
				0x30 >> 1),
		.platform_data = &gsensor_bma250_platform_data,
		.irq = MSM_GPIO_TO_INT(GOLFU_GPIO_GSENSORS_INT),
	},
};
static uint8_t cm3629_mapping_table[] = {0x0, 0x3, 0x6, 0x9, 0xC,
			0xF, 0x12, 0x15, 0x18, 0x1B,
			0x1E, 0x21, 0x24, 0x27, 0x2A,
			0x2D, 0x30, 0x33, 0x36, 0x39,
			0x3C, 0x3F, 0x43, 0x47, 0x4B,
			0x4F, 0x53, 0x57, 0x5B, 0x5F,
			0x63, 0x67, 0x6B, 0x70, 0x75,
			0x7A, 0x7F, 0x84, 0x89, 0x8E,
			0x93, 0x98, 0x9D, 0xA2, 0xA8,
			0xAE, 0xB4, 0xBA, 0xC0, 0xC6,
			0xCC, 0xD3, 0xDA, 0xE1, 0xE8,
			0xEF, 0xF6, 0xFF};

static struct cm3629_platform_data cm36282_pdata = {
	.model = CAPELLA_CM36282,
	.ps_select = CM3629_PS1_ONLY,
	.intr = GOLFU_GPIO_PROXIMITY_INT,
	.levels = {1,4,7,35,77,1985,3321,3985,4648,65535},
    .golden_adc = 0x7BC,
    .power = NULL,
	.ps_calibration_rule = 1,
    .cm3629_slave_address = 0xC0>>1,
    .ps1_thd_set = 21,
    .ps1_thd_no_cal = 0xF1,
    .ps1_thd_with_cal = 14,
	.ps1_thh_diff = 2,
	.ps_conf1_val = CM3629_PS_DR_1_80 | CM3629_PS_IT_1_6T |
                        CM3629_PS1_PERS_3,
    .ps_conf2_val = CM3629_PS_ITB_1 | CM3629_PS_ITR_1 |
                        CM3629_PS2_INT_DIS | CM3629_PS1_INT_DIS,
    .ps_conf3_val = CM3629_PS2_PROL_32,
    .enable_polling_ignore = 1,
	.mapping_table = cm3629_mapping_table,
	.mapping_size = ARRAY_SIZE(cm3629_mapping_table),
};

static struct i2c_board_info i2c_cm36282_devices[] = {
	{
		I2C_BOARD_INFO(CM3629_I2C_NAME,0xC0 >> 1),
		.platform_data = &cm36282_pdata,
		.irq = MSM_GPIO_TO_INT(GOLFU_GPIO_PROXIMITY_INT),
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
	SND(MONO_HEADSET, 2),
	SND(HEADSET, 3),
	SND(SPEAKER, 6),
	SND(TTY_HEADSET, 8),
	SND(TTY_VCO, 9),
	SND(TTY_HCO, 10),
	SND(BT, 12),
	SND(IN_S_SADC_OUT_HANDSET, 16),
	SND(IN_S_SADC_OUT_SPEAKER_PHONE, 25),
	SND(FM_DIGITAL_STEREO_HEADSET, 26),
	SND(FM_DIGITAL_SPEAKER_PHONE, 27),
	SND(FM_DIGITAL_BT_A2DP_HEADSET, 28),
	SND(CURRENT, 0x7FFFFFFE),
	SND(FM_ANALOG_STEREO_HEADSET, 35),
	SND(FM_ANALOG_STEREO_HEADSET_CODEC, 36),
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
#if 0
static struct smsc911x_platform_config smsc911x_config = {
	.irq_polarity	= SMSC911X_IRQ_POLARITY_ACTIVE_HIGH,
	.irq_type	= SMSC911X_IRQ_TYPE_PUSH_PULL,
	.flags		= SMSC911X_USE_16BIT,
};

static struct resource smsc911x_resources[] = {
	[0] = {
		.start	= 0x90000000,
		.end	= 0x90007fff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= MSM_GPIO_TO_INT(48),
		.end	= MSM_GPIO_TO_INT(48),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
	},
};

static struct platform_device smsc911x_device = {
	.name		= "smsc911x",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(smsc911x_resources),
	.resource	= smsc911x_resources,
	.dev		= {
		.platform_data	= &smsc911x_config,
	},
};

static struct msm_gpio smsc911x_gpios[] = {
	{ GPIO_CFG(48, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
							 "smsc911x_irq"  },
	{ GPIO_CFG(49, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
							 "eth_fifo_sel" },
};

#define ETH_FIFO_SEL_GPIO	49
static void msm7x27a_cfg_smsc911x(void)
{
	int res;

	res = msm_gpios_request_enable(smsc911x_gpios,
				 ARRAY_SIZE(smsc911x_gpios));
	if (res) {
		pr_err("%s: unable to enable gpios for SMSC911x\n", __func__);
		return;
	}

	/* ETH_FIFO_SEL */
	res = gpio_direction_output(ETH_FIFO_SEL_GPIO, 0);
	if (res) {
		pr_err("%s: unable to get direction for gpio %d\n", __func__,
							 ETH_FIFO_SEL_GPIO);
		msm_gpios_disable_free(smsc911x_gpios,
						 ARRAY_SIZE(smsc911x_gpios));
		return;
	}
	gpio_set_value(ETH_FIFO_SEL_GPIO, 0);
}
#endif
#ifdef CONFIG_MSM_CAMERA
static uint32_t camera_off_gpio_table[] = {
	/* HTC_START */
	/* sleep status */
	GPIO_CFG(GOLFU_GPIO_CAMERA_SDA,  1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GOLFU_GPIO_CAMERA_SCL,   1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GOLFU_GPIO_CAMERA_RESET,     0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GOLFU_GPIO_CAMERA_STANDBY,   0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GOLFU_GPIO_CAMERA_MCLK,      0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),
	GPIO_CFG(GOLFU_GPIO_CAM_D1V8_EN,	 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GOLFU_GPIO_CAM_A2V85_EN,	 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	/* HTC_END */
};

static uint32_t camera_on_gpio_table[] = {
	/* HTC_START */
	/* sleep status */
	GPIO_CFG(GOLFU_GPIO_CAMERA_SDA,  1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GOLFU_GPIO_CAMERA_SCL,   1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GOLFU_GPIO_CAMERA_RESET,     0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GOLFU_GPIO_CAMERA_STANDBY,   0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GOLFU_GPIO_CAMERA_MCLK,      1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),
	GPIO_CFG(GOLFU_GPIO_CAM_D1V8_EN,	 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GOLFU_GPIO_CAM_A2V85_EN,	 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	/* HTC_END */
};

//HTC_START
//For Golf#U camera power control
static struct vreg *vreg_usim2;
static struct vreg *vreg_wlan1p2c50; /* PMIC L6 1.5V for XB board */

static void golfu_camera_vreg_config(int vreg_en)
{
	int rc;

	printk("golfu_camera_vreg_config %d\n", vreg_en);

	if (vreg_usim2 == NULL) { //IO 1.8v L15
		vreg_usim2 = vreg_get(NULL, "usim2");
		if (IS_ERR(vreg_usim2)) {
			pr_err("%s: vreg_get(%s) failed (%ld)\n",
			__func__, "usim2", PTR_ERR(vreg_usim2));
			return;
		}

		printk("camera_vreg_config: vreg_usim2 1800\n");
		rc = vreg_set_level(vreg_usim2, 1800);
		if (rc) {
			pr_err("%s: camera_usim2 set_level failed (%d)\n",
			__func__, rc);
		}
	}

	/* For XB board and later */
	if(system_rev >= 0x1){
		if (vreg_wlan1p2c50 == NULL) { //D1.5V from PMIC L6
			vreg_wlan1p2c50 = vreg_get(NULL, "wlan3");
			if (IS_ERR(vreg_wlan1p2c50)) {
				pr_err("%s: vreg_get(%s) failed (%ld)\n",
				__func__, "vreg_wlan1p2c50", PTR_ERR(vreg_wlan1p2c50));
				return;
			}

			printk("camera_vreg_config: vreg_wlan1p2c50 1500\n");
			rc = vreg_set_level(vreg_wlan1p2c50, 1500);
			if (rc) {
				pr_err("%s: vreg_wlan1p2c50 set_level failed (%d)\n",
				__func__, rc);
			}
		}
	}/* (system_rev >= 0x1) End*/

	if (vreg_en) {
		if(system_rev >= 0x1){ /* for XB board with 5M and later*/
			/* D1V5 for 5M 4e1 */
			rc = vreg_enable(vreg_wlan1p2c50);
			pr_info("vreg_wlan1p2c50 enable D1V5\n");
			if (rc) {
				pr_err("%s: vreg_wlan1p2c50 enable failed (%d)\n",
				__func__, rc);
			}
			udelay(5);

			/* A2V85 */
			gpio_set_value(GOLFU_GPIO_CAM_A2V85_EN, 1);
			pr_info("enable A2V85\n");
			udelay(5);

			/* IO 1V8 */
			rc = vreg_enable(vreg_usim2);
			pr_info("vreg_usim2 enable IO 1V8\n");
			if (rc) {
				pr_err("%s: USIM enable failed (%d)\n",
				__func__, rc);
			}
		}else{ /* for XA board with 3M */
			/* IO 1V8 */
			rc = vreg_enable(vreg_usim2);
			pr_info("vreg_usim2 enable IO 1V8\n");
			if (rc) {
				pr_err("%s: USIM enable failed (%d)\n",
					__func__, rc);
			}
			udelay(5);

			/* D1V8 for 3M */
			gpio_set_value(GOLFU_GPIO_CAM_D1V8_EN, 1);
			pr_info("enable D1V8\n");
			udelay(5);

			/* A2V85 */
			gpio_set_value(GOLFU_GPIO_CAM_A2V85_EN, 1);
			pr_info("enable A2V85\n");
		}
	} else {
		if(system_rev >= 0x1){ /* for XB board with 5M and later */
			/* IO 1V8 */
			rc = vreg_disable(vreg_usim2);
			if (rc) {
				pr_err("%s: IO 1V8 disable failed (%d)\n",
				__func__, rc);
			}
			pr_info("%s: IO 1V8 disable OK\n", __func__);
			hr_msleep(5);

			/* A2V85 */
			gpio_set_value(GOLFU_GPIO_CAM_A2V85_EN, 0);
			pr_info("disable A2V85\n");
			hr_msleep(5);

			/* D1V5 for XB board with 5M and later */
			rc = vreg_disable(vreg_wlan1p2c50);
			if (rc) {
				pr_err("%s: D1V5 vreg_wlan1p2c50 disable failed (%d)\n",
				__func__, rc);
			}
			pr_info("%s: D1V5 vreg_wlan1p2c50 disable OK\n", __func__);
		}else{ /* for XA board with 3M */
			/* A2V85 */
			gpio_set_value(GOLFU_GPIO_CAM_A2V85_EN, 0);
			pr_info("disable A2V85\n");
			hr_msleep(5);

			/* D1V8 */
			gpio_set_value(GOLFU_GPIO_CAM_D1V8_EN, 0);
			pr_info("disable D1V8\n");
			hr_msleep(5);

			/* IO 1V8 */
			rc = vreg_disable(vreg_usim2);
			if (rc) {
				pr_err("%s: IO 1V8 disable failed (%d)\n",
					__func__, rc);
			}
			pr_info("%s: IO 1V8 disable OK\n", __func__);
		}
	}
}
//HTC_END

#ifdef CONFIG_MSM_CAMERA_FLASH
static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_EXT,
	._fsrc.ext_driver_src.led_en = GPIO_CAM_GP_LED_EN1,
	._fsrc.ext_driver_src.led_flash_en = GPIO_CAM_GP_LED_EN2,
};
#endif
static struct regulator_bulk_data regs_camera[] = {
	{ .supply = "msme1", .min_uV = 1800000, .max_uV = 1800000 },
	{ .supply = "gp2",   .min_uV = 2850000, .max_uV = 2850000 },
	{ .supply = "usb2",  .min_uV = 1800000, .max_uV = 1800000 },
};

#if 0 /* not use */
static void __init msm_camera_vreg_init(void)
{
	int rc;

	rc = regulator_bulk_get(NULL, ARRAY_SIZE(regs_camera), regs_camera);

	if (rc) {
		pr_err("%s: could not get regulators: %d\n", __func__, rc);
		return;
	}

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs_camera), regs_camera);

	if (rc) {
		pr_err("%s: could not set voltages: %d\n", __func__, rc);
		return;
	}
}
#endif

static void msm_camera_vreg_config(int vreg_en)
{
	int rc = vreg_en ?
		regulator_bulk_enable(ARRAY_SIZE(regs_camera), regs_camera) :
		regulator_bulk_disable(ARRAY_SIZE(regs_camera), regs_camera);

	if (rc)
		pr_err("%s: could not %sable regulators: %d\n",
				__func__, vreg_en ? "en" : "dis", rc);
}

/* HTC_START 20110804 */
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

static int config_camera_on_gpios_rear(void)
{
	int rc = 0;

	if (machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa())
		msm_camera_vreg_config(1);

//HTC_START
//Waiting HW to confirm pico need cut off power.
#ifdef CONFIG_ARCH_MSM7X27A
	golfu_camera_vreg_config(1);
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
	golfu_camera_vreg_config(0);
#endif
//HTC_END

	config_gpio_table(camera_off_gpio_table,
			ARRAY_SIZE(camera_off_gpio_table));
}

static int config_camera_on_gpios_front(void)
{
	int rc = 0;

	if (machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa())
		msm_camera_vreg_config(1);

	rc = config_gpio_table(camera_on_gpio_table,
			ARRAY_SIZE(camera_on_gpio_table));
	if (rc < 0) {
		pr_err("%s: CAMSENSOR gpio table request"
			"failed\n", __func__);
		return rc;
	}

	return rc;
}

static void config_camera_off_gpios_front(void)
{
	if (machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa())
		msm_camera_vreg_config(0);

	config_gpio_table(camera_off_gpio_table,
			ARRAY_SIZE(camera_off_gpio_table));
}

struct msm_camera_device_platform_data golfu_camera_device_data_rear = {
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

struct msm_camera_device_platform_data golfu_camera_device_data_front = {
	.camera_gpio_on  = config_camera_on_gpios_front,
	.camera_gpio_off = config_camera_off_gpios_front,
	.ioext.csiphy = 0xA0F00000,
	.ioext.csisz  = 0x00100000,
	.ioext.csiirq = INT_CSI_IRQ_0,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 192000000,
	.ioext.appphy = MSM_CLK_CTL_PHYS,
	.ioext.appsz  = MSM_CLK_CTL_SIZE,
};
/* HTC_START */
#ifdef CONFIG_MT9T013
static struct msm_camera_sensor_platform_info mt9t013_sensor_7627a_info = {
	.mount_angle = 0
};


static struct msm_camera_sensor_flash_data flash_mt9t013 = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src  = &msm_flash_src
#endif
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9t013_data = {
	.sensor_name    = "mt9t013",
	.sensor_reset   = GOLFU_GPIO_CAMERA_RESET,
	.sensor_pwd     = GOLFU_GPIO_CAMERA_STANDBY,
	.mclk           = GOLFU_GPIO_CAMERA_MCLK,
	.pdata          = &golfu_camera_device_data_rear,
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
/*HTC_END */

#ifdef CONFIG_S5K4E1
static struct msm_camera_sensor_platform_info s5k4e1_sensor_7627a_info = {
	.mount_angle = 90
};

static struct msm_camera_sensor_flash_data flash_s5k4e1 = {
	.flash_type             = MSM_CAMERA_FLASH_NONE,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src              = &msm_flash_src
#endif
};
static struct msm_camera_sensor_info msm_camera_sensor_s5k4e1_data;
static struct msm_camera_sensor_info msm_camera_sensor_s5k4e1_data = {
	.sensor_name    = "s5k4e1",
/*	.sensor_reset_enable = 1, */
	.sensor_reset   = GOLFU_GPIO_CAMERA_RESET,
/*	.sensor_pwd             = 85, */
/*	.vcm_pwd                = 0, */
	.vcm_enable             = 0, /* 5M FF from XB board*/
	.pdata                  = &golfu_camera_device_data_rear,
	.flash_data             = &flash_s5k4e1,
	.sensor_platform_info   = &s5k4e1_sensor_7627a_info,
	.full_size_preview		= true,
	.csi_if                 = 1
};

static struct platform_device msm_camera_sensor_s5k4e1 = {
	.name   = "msm_camera_s5k4e1",
	.dev    = {
		.platform_data = &msm_camera_sensor_s5k4e1_data,
	},
};
#endif

#ifdef CONFIG_IMX072
static struct msm_camera_sensor_platform_info imx072_sensor_7627a_info = {
	.mount_angle = 90
};

static struct msm_camera_sensor_flash_data flash_imx072 = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
	.flash_src              = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_imx072_data = {
	.sensor_name    = "imx072",
	.sensor_reset_enable = 1,
	.sensor_reset   = GPIO_CAM_GP_CAMIF_RESET_N, /* TODO 106,*/
	.sensor_pwd             = 85,
	.vcm_pwd                = GPIO_CAM_GP_CAM_PWDN,
	.vcm_enable             = 1,
	.pdata                  = &golfu_camera_device_data_rear,
	.flash_data             = &flash_imx072,
	.sensor_platform_info = &imx072_sensor_7627a_info,
	.csi_if                 = 1
};

static struct platform_device msm_camera_sensor_imx072 = {
	.name   = "msm_camera_imx072",
	.dev    = {
		.platform_data = &msm_camera_sensor_imx072_data,
	},
};
#endif

#ifdef CONFIG_WEBCAM_OV9726
static struct msm_camera_sensor_platform_info ov9726_sensor_7627a_info = {
	.mount_angle = 90
};

static struct msm_camera_sensor_flash_data flash_ov9726 = {
	.flash_type             = MSM_CAMERA_FLASH_NONE,
	.flash_src              = &msm_flash_src
};
static struct msm_camera_sensor_info msm_camera_sensor_ov9726_data;
static struct msm_camera_sensor_info msm_camera_sensor_ov9726_data = {
	.sensor_name    = "ov9726",
	.sensor_reset_enable = 0,
	.sensor_reset   = GPIO_CAM_GP_CAM1MP_XCLR,
	.sensor_pwd             = 85,
	.vcm_pwd                = 1,
	.vcm_enable             = 0,
	.pdata                  = &golfu_camera_device_data_front,
	.flash_data             = &flash_ov9726,
	.sensor_platform_info   = &ov9726_sensor_7627a_info,
	.csi_if                 = 1
};

static struct platform_device msm_camera_sensor_ov9726 = {
	.name   = "msm_camera_ov9726",
	.dev    = {
		.platform_data = &msm_camera_sensor_ov9726_data,
	},
};
#else
/*static inline void msm_camera_vreg_init(void) { }*/
#endif

#ifdef CONFIG_MT9E013
static struct msm_camera_sensor_platform_info mt9e013_sensor_7627a_info = {
	.mount_angle = 90
};

static struct msm_camera_sensor_flash_data flash_mt9e013 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9e013_data = {
	.sensor_name    = "mt9e013",
	.sensor_reset   = 0,
	.sensor_reset_enable = 1,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable     = 0,
	.pdata          = &golfu_camera_device_data_rear,
	.flash_data     = &flash_mt9e013,
	.sensor_platform_info   = &mt9e013_sensor_7627a_info,
	.csi_if         = 1
};

static struct platform_device msm_camera_sensor_mt9e013 = {
	.name      = "msm_camera_mt9e013",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9e013_data,
	},
};
#endif

static struct i2c_board_info i2c_camera_3M_devices[] = {
/* HTC_START */
	#ifdef CONFIG_MT9T013
	{
		I2C_BOARD_INFO("mt9t013", 0x6C ),
	},
	#endif
/* HTC_END */
};

static struct i2c_board_info i2c_camera_5M_devices[] = {
	#ifdef CONFIG_S5K4E1
	{
		I2C_BOARD_INFO("s5k4e1", 0x20 >>1),
	},
/* for 5M FF s5k4e1 sensor
	{
		I2C_BOARD_INFO("s5k4e1_af", 0x18 >>1),
	},
*/
	#endif
};
#endif /* CONFIG_MSM_CAMERA define END */
#if defined(CONFIG_SERIAL_MSM_HSL_CONSOLE) \
		&& defined(CONFIG_MSM_SHARED_GPIO_FOR_UART2DM)
static struct msm_gpio uart2dm_gpios[] = {
	{GPIO_CFG(19, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
							"uart2dm_rfr_n" },
	{GPIO_CFG(20, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
							"uart2dm_cts_n" },
	{GPIO_CFG(21, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
							"uart2dm_rx"    },
	{GPIO_CFG(108, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
							"uart2dm_tx"    },
};

static void msm7x27a_cfg_uart2dm_serial(void)
{
	int ret;
	ret = msm_gpios_request_enable(uart2dm_gpios,
					ARRAY_SIZE(uart2dm_gpios));
	if (ret)
		pr_err("%s: unable to enable gpios for uart2dm\n", __func__);
}
#else
static void msm7x27a_cfg_uart2dm_serial(void) { }
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

#if 0
static struct platform_device *rumi_sim_devices[] __initdata = {
	&msm_device_dmov,
	&msm_device_smd,
	&smc91x_device,
	&msm_device_uart1,
	&msm_device_nand,
	&msm_device_uart_dm1,
	&msm_gsbi0_qup_i2c_device,
	&msm_gsbi1_qup_i2c_device,
};

static struct platform_device *golfu_devices[] __initdata = {
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
/*	&msm_device_adspdec,*/
#ifdef CONFIG_BATTERY_MSM
	&msm_batt_device,
#endif
/*	&htc_headset_mgr,*/
#ifdef CONFIG_S5K4E1
/*	&msm_camera_sensor_s5k4e1,*/
#endif
#ifdef CONFIG_IMX072
/*	&msm_camera_sensor_imx072,*/
#endif
#ifdef CONFIG_WEBCAM_OV9726
/*	&msm_camera_sensor_ov9726,*/
#endif
#ifdef CONFIG_MT9E013
/*	&msm_camera_sensor_mt9e013,*/
#endif
	&msm_kgsl_3d0,
#ifdef CONFIG_BT
	/*&msm_bt_power_device,*/
#endif
#ifdef CONFIG_MT9T013
/*	&msm_camera_sensor_mt9t013,*/
#endif
#ifdef CONFIG_BT
/*	&wifi_bt_slp_clk,*/
/*	&golfu_rfkill,*/
/*	&msm_device_uart_dm1,*/
#endif
/*	&pm8029_leds,*/
};

#else
static struct platform_device *golfu_devices[] __initdata = {
	&ram_console_device,
	&msm_device_dmov,
	&msm_device_smd,
	/* &msm_device_uart1, */
	&msm_device_uart3,
	&msm_device_uart_dm1,
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
/*	&lcdc_toshiba_panel_device,*/
	&msm_batt_device,
	&htc_headset_mgr,
	&htc_drm,
/*	&smsc911x_device,*/
/* //move blow for XA/XB board
#ifdef CONFIG_S5K4E1
	&msm_camera_sensor_s5k4e1,
#endif
*/
#ifdef CONFIG_IMX072
/*	&msm_camera_sensor_imx072,*/
#endif
#ifdef CONFIG_WEBCAM_OV9726
/*	&msm_camera_sensor_ov9726,*/
#endif
#ifdef CONFIG_MT9E013
/*	&msm_camera_sensor_mt9e013,*/
#endif
/* HTC_START */
/* //move blow for XA/XB board
#ifdef CONFIG_MT9T013
	&msm_camera_sensor_mt9t013,
#endif
*/
/* HTC_END */
#ifdef CONFIG_FB_MSM_MIPI_DSI
/*	&mipi_dsi_renesas_panel_device,*/
#endif
	&msm_kgsl_3d0,
#ifdef CONFIG_BT
	&msm_bt_power_device,
#endif
	&cable_detect_device,
/*	&asoc_msm_pcm,*/
/*	&asoc_msm_dai0,*/
/*	&asoc_msm_dai1,*/
};
#endif

/* for XA board with 3M and XB board with 5M Camera */
#if defined(CONFIG_MSM_CAMERA)
static struct platform_device *golfu_camera_5M_devices[] __initdata = {
	&msm_camera_sensor_s5k4e1,
};

static struct platform_device *golfu_camera_3M_devices[] __initdata = {
	&msm_camera_sensor_mt9t013,
};
#endif

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
		gpio_set_value(GOLFU_V_TP_3V3_EN, 1);
#endif
	return 0;
}

static void touch_config_pins(int state)
{
	switch (state) {
	case TP_INIT:
		gpio_tlmm_config(GPIO_CFG(GOLFU_GPIO_TP_ATT_N, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(GOLFU_GPIO_TP_RST_N, 0,
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
	.gpio_irq	= GOLFU_GPIO_TP_ATT_N,
	.gpio_reset	= GOLFU_GPIO_TP_RST_N,
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

static struct synaptics_i2c_rmi_platform_data golfu_ts_synaptics_data[] = { /* Synatpics sensor */
	{
		.version = 0x3330,
		.abs_x_min = 0,
		.abs_x_max = 1100,
		.abs_y_min = 0,
		.abs_y_max = 1720,
		.gpio_irq = GOLFU_GPIO_TP_ATT_N,
		.gpio_reset = GOLFU_GPIO_TP_RST_N,
		.packrat_number = 1100756,
		.default_config = 3,
		.large_obj_check = 1,
		.tw_pin_mask = 0x0180,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.reduce_report_level = {30, 30, 50, 0, 0},
		.customer_register = {0xF9, 0x64, 0x04, 0x64},
		.config = {0x47, 0x4F, 0x46, 0x43, 0x00, 0x3F, 0x03, 0x1E,
			0x05, 0xB1, 0x09, 0x0B, 0x05, 0x05, 0x00, 0x00,
			0x4C, 0x04, 0x6C, 0x07, 0x28, 0x05, 0x2D, 0x80,
			0x11, 0x55, 0x01, 0x01, 0x16, 0x02, 0x19, 0x01,
			0x42, 0x50, 0x3F, 0x49, 0xB0, 0xB3, 0xC8, 0xAF,
			0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x08, 0x04,
			0xBC, 0x57, 0x1E, 0xA2, 0x02, 0x37, 0x02, 0x02,
			0x96, 0x12, 0x0A, 0x00, 0x02, 0x04, 0x01, 0xC0,
			0x03, 0x0E, 0x1F, 0x12, 0x3A, 0x00, 0x19, 0x08,
			0x00, 0x00, 0x10, 0xFF, 0x00, 0x00, 0x01, 0x02,
			0x04, 0x06, 0x09, 0x0C, 0x0E, 0x10, 0x1B, 0x1A,
			0x19, 0x18, 0x16, 0x15, 0x14, 0x12, 0x11, 0x13,
			0x12, 0x10, 0x0E, 0x0C, 0x0A, 0x08, 0x06, 0x04,
			0x02, 0xFF, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,
			0xC0, 0xC0, 0x56, 0x55, 0x53, 0x51, 0x50, 0x4E,
			0x4C, 0x4A, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0B,
			0x0E, 0x11, 0x00, 0xA0, 0x0F, 0xE6, 0x28, 0x00,
			0x20, 0x4E, 0xB3, 0xC8, 0x80, 0xA0, 0x0F, 0x00,
			0xC0, 0x80, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10,
			0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x60,
			0x63, 0x65, 0x67, 0x69, 0x6C, 0x6F, 0x72, 0x00,
			0x2C, 0x01, 0x10, 0x0A, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04},
	},
	{
		.version = 0x3330,
		.abs_x_min = 0,
		.abs_x_max = 1100,
		.abs_y_min = 0,
		.abs_y_max = 1720,
		.gpio_irq = GOLFU_GPIO_TP_ATT_N,
		.gpio_reset = GOLFU_GPIO_TP_RST_N,
		.packrat_number = 1091744,
		.default_config = 3,
		.large_obj_check = 1,
		.tw_pin_mask = 0x0180,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.config = {0x47, 0x4F, 0x46, 0x34, 0x80, 0x3F, 0x03, 0x1E,
			0x05, 0xB1, 0x09, 0x0B, 0x05, 0x05, 0x00, 0x00,
			0x4C, 0x04, 0x6C, 0x07, 0x1E, 0x05, 0x2D, 0x80,
			0x11, 0x55, 0x01, 0x01, 0x16, 0x02, 0x19, 0x01,
			0x42, 0x50, 0x3F, 0x49, 0x20, 0xBF, 0xDB, 0xB9,
			0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x04,
			0xBC, 0x47, 0x1E, 0xA2, 0x02, 0x37, 0x02, 0x02,
			0x96, 0x12, 0x0A, 0x00, 0x02, 0x3F, 0x01, 0x80,
			0x02, 0x0E, 0x1F, 0x12, 0x3A, 0x00, 0x19, 0x08,
			0x00, 0x00, 0x10, 0xFF, 0x00, 0x00, 0x01, 0x02,
			0x04, 0x06, 0x09, 0x0C, 0x0E, 0x10, 0x1B, 0x1A,
			0x19, 0x18, 0x16, 0x15, 0x14, 0x12, 0x11, 0x13,
			0x12, 0x10, 0x0E, 0x0C, 0x0A, 0x08, 0x06, 0x04,
			0x02, 0xFF, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,
			0xC0, 0xC0, 0x56, 0x55, 0x53, 0x51, 0x50, 0x4E,
			0x4C, 0x4A, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0B,
			0x0E, 0x11, 0x00, 0xA0, 0x0F, 0xE6, 0x28, 0x00,
			0x20, 0x4E, 0xB3, 0xC8, 0x80, 0xA0, 0x0F, 0x00,
			0xC0, 0x80, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10,
			0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x60,
			0x63, 0x65, 0x67, 0x69, 0x6C, 0x6F, 0x72, 0x00,
			0x64, 0x00, 0x10, 0x0A, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04},
	},
	{
		.version = 0x3330,
		.abs_x_min = 0,
		.abs_x_max = 1090,
		.abs_y_min = 0,
		.abs_y_max = 1629,
		.gpio_irq = GOLFU_GPIO_TP_ATT_N,
		.gpio_reset = GOLFU_GPIO_TP_RST_N,
		.default_config = 3,
		.large_obj_check = 1,
		.tw_pin_mask = 0x0180,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.config = {0x48, 0x30, 0x30, 0x30, 0x04, 0x3F, 0x03, 0x1E,
			0x05, 0xB1, 0x08, 0x0B, 0x19, 0x19, 0x00, 0x00,
			0x4C, 0x04, 0x6C, 0x07, 0x1E, 0x05, 0x28, 0xF5,
			0x28, 0x1E, 0x05, 0x01, 0x30, 0x00, 0x30, 0x00,
			0x00, 0x48, 0x00, 0x48, 0xF0, 0xD2, 0xF0, 0xD2,
			0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x04,
			0xC0, 0x00, 0x08, 0xA2, 0x02, 0x32, 0x0A, 0x0A,
			0x96, 0x12, 0x0A, 0x00, 0x02, 0xF4, 0x01, 0x80,
			0x03, 0x0E, 0x1F, 0x00, 0xDB, 0x01, 0x19, 0x04,
			0x00, 0x00, 0x10, 0xFF, 0x00, 0x00, 0x01, 0x02,
			0x04, 0x06, 0x09, 0x0C, 0x0E, 0x10, 0x1B, 0x1A,
			0x19, 0x18, 0x16, 0x15, 0x14, 0x12, 0x11, 0x13,
			0x12, 0x10, 0x0E, 0x0C, 0x0A, 0x08, 0x06, 0x04,
			0x02, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
			0xC0, 0x80, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10,
			0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x08, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x10,
			0x0A, 0x00, 0x00},
	},
	{
		.version = 0x3230,
		.abs_x_min = 0,
		.abs_x_max = 1090,
		.abs_y_min = 81,
		.abs_y_max = 1629,
		.gpio_irq = GOLFU_GPIO_TP_ATT_N,
		.gpio_reset = GOLFU_GPIO_TP_RST_N,
		.default_config = 1,
		.large_obj_check = 1,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.config = {0x30, 0x30, 0x30, 0x33, 0x00, 0x0F, 0x03, 0x1E,
			0x05, 0x20, 0xB1, 0x00, 0x0B, 0x19, 0x19, 0x00,
			0x00, 0x4C, 0x04, 0x6C, 0x07, 0x1E, 0x05, 0x28,
			0xF5, 0x28, 0x1E, 0x05, 0x01, 0x30, 0x00, 0x30,
			0x00, 0x00, 0x48, 0x00, 0x48, 0xF0, 0xD2, 0xF0,
			0xD2, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x0A,
			0x04, 0xC0, 0x00, 0x02, 0xF4, 0x01, 0x80, 0x03,
			0x0E, 0x1F, 0x00, 0xDB, 0x01, 0x19, 0x04, 0x1E,
			0x00, 0x10, 0x0A, 0x00, 0x00, 0x01, 0x02, 0x04,
			0x06, 0x09, 0x0C, 0x0E, 0x10, 0x1B, 0x1A, 0x19,
			0x18, 0x16, 0x15, 0x14, 0x12, 0x11, 0x13, 0x12,
			0x10, 0x0E, 0x0C, 0x0A, 0x08, 0x06, 0x04, 0x02,
			0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xC0,
			0x80, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x08, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00},
	},
	{
		.version = 0x0000,
		.abs_x_min = 35,
		.abs_x_max = 965,
		.abs_y_min = 0,
		.abs_y_max = 1770,
		.flags = SYNAPTICS_FLIP_Y,
		.gpio_irq = GOLFU_GPIO_TP_ATT_N,
		.gpio_reset = GOLFU_GPIO_TP_RST_N,
		.default_config = 2,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.config = {0x30, 0x30, 0x30, 0x31, 0x84, 0x0F, 0x03, 0x1E,
			0x05, 0x00, 0x0B, 0x19, 0x19, 0x00, 0x00, 0xE8,
			0x03, 0x75, 0x07, 0x1E, 0x05, 0x28, 0xF5, 0x28,
			0x1E, 0x05, 0x01, 0x30, 0x00, 0x30, 0x00, 0x00,
			0x48, 0x00, 0x48, 0x0D, 0xD6, 0x56, 0xBE, 0x00,
			0x70, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x04, 0x00,
			0x02, 0xCD, 0x00, 0x80, 0x03, 0x0D, 0x1F, 0x00,
			0x21, 0x00, 0x15, 0x04, 0x1E, 0x00, 0x10, 0x02,
			0x01, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
			0x19, 0x1A, 0x1B, 0x11, 0xFF, 0xFF, 0xFF, 0x03,
			0x05, 0x07, 0x13, 0x11, 0x0F, 0x0D, 0x0B, 0x0A,
			0x09, 0x08, 0x01, 0x02, 0x04, 0x06, 0x0C, 0x0E,
			0x10, 0x12, 0xFF, 0xC0, 0x88, 0xC0, 0x88, 0xC0,
			0x88, 0xC0, 0x88, 0x3A, 0x34, 0x3A, 0x34, 0x3A,
			0x34, 0x3C, 0x34, 0x00, 0x04, 0x08, 0x0C, 0x1E,
			0x14, 0x3C, 0x1E, 0x00, 0x9B, 0x7F, 0x46, 0x20,
			0x4E, 0x9B, 0x7F, 0x28, 0x80, 0xCC, 0xF4, 0x01,
			0x00, 0xC0, 0x80, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x30, 0x30, 0x00, 0x1E, 0x19,
			0x05, 0x00, 0x00, 0x3D, 0x08, 0x00, 0x00, 0x00,
			0xBC, 0x02, 0x80},
	},
};



static struct i2c_board_info i2c_touch_device[] = {
	{
		I2C_BOARD_INFO(SYNAPTICS_3200_NAME, 0x40>>1),
		.platform_data = &golfu_ts_synaptics_data,
		.irq = MSM_GPIO_TO_INT(GOLFU_GPIO_TP_ATT_N),
	},
#if defined(CONFIG_TOUCHSCREEN_HIMAX_SH)
	{
		I2C_BOARD_INFO("hx8526-a", 0x48),
		.platform_data  = &hx8526a_pdata,
	},
#endif
};

static ssize_t golfu_virtual_keys_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
		__stringify(EV_KEY) ":" __stringify(KEY_BACK)	    ":45:526:60:65"
		":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":160:526:68:65"
		":" __stringify(EV_KEY) ":" __stringify(KEY_APP_SWITCH) ":275:526:62:65"
		"\n");
}

static struct kobj_attribute golfu_synaptics_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.synaptics-rmi-touchscreen",
		.mode = S_IRUGO,
	},
	.show = &golfu_virtual_keys_show,
};

static struct attribute *golfu_properties_attrs[] = {
	&golfu_synaptics_virtual_keys_attr.attr,
	NULL
};

static struct attribute_group golfu_properties_attr_group = {
	.attrs = golfu_properties_attrs,
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
#if 0
#define KP_INDEX(row, col) ((row)*ARRAY_SIZE(kp_col_gpios) + (col))

static unsigned int kp_row_gpios[] = {31, 32, 33, 34, 35};
static unsigned int kp_col_gpios[] = {36, 37, 38, 39, 40};

static const unsigned short keymap[ARRAY_SIZE(kp_col_gpios) *
					  ARRAY_SIZE(kp_row_gpios)] = {
	[KP_INDEX(0, 0)] = KEY_7,
	[KP_INDEX(0, 1)] = KEY_DOWN,
	[KP_INDEX(0, 2)] = KEY_UP,
	[KP_INDEX(0, 3)] = KEY_RIGHT,
	[KP_INDEX(0, 4)] = KEY_ENTER,

	[KP_INDEX(1, 0)] = KEY_LEFT,
	[KP_INDEX(1, 1)] = KEY_SEND,
	[KP_INDEX(1, 2)] = KEY_1,
	[KP_INDEX(1, 3)] = KEY_4,
	[KP_INDEX(1, 4)] = KEY_CLEAR,

	[KP_INDEX(2, 0)] = KEY_6,
	[KP_INDEX(2, 1)] = KEY_5,
	[KP_INDEX(2, 2)] = KEY_8,
	[KP_INDEX(2, 3)] = KEY_3,
	[KP_INDEX(2, 4)] = KEY_NUMERIC_STAR,

	[KP_INDEX(3, 0)] = KEY_9,
	[KP_INDEX(3, 1)] = KEY_NUMERIC_POUND,
	[KP_INDEX(3, 2)] = KEY_0,
	[KP_INDEX(3, 3)] = KEY_2,
	[KP_INDEX(3, 4)] = KEY_SLEEP,

	[KP_INDEX(4, 0)] = KEY_BACK,
	[KP_INDEX(4, 1)] = KEY_HOME,
	[KP_INDEX(4, 2)] = KEY_MENU,
	[KP_INDEX(4, 3)] = KEY_VOLUMEUP,
	[KP_INDEX(4, 4)] = KEY_VOLUMEDOWN,
};

/* SURF keypad platform device information */
static struct gpio_event_matrix_info kp_matrix_info = {
	.info.func	= gpio_event_matrix_func,
	.keymap		= keymap,
	.output_gpios	= kp_row_gpios,
	.input_gpios	= kp_col_gpios,
	.noutputs	= ARRAY_SIZE(kp_row_gpios),
	.ninputs	= ARRAY_SIZE(kp_col_gpios),
	.settle_time.tv_nsec = 40 * NSEC_PER_USEC,
	.poll_time.tv_nsec = 20 * NSEC_PER_MSEC,
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
			  GPIOKPF_PRINT_UNMAPPED_KEYS,
};

static struct gpio_event_info *kp_info[] = {
	&kp_matrix_info.info
};

static struct gpio_event_platform_data kp_pdata = {
	.name		= "7x27a_kp",
	.info		= kp_info,
	.info_count	= ARRAY_SIZE(kp_info)
};

static struct platform_device kp_pdev = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &kp_pdata,
	},
};
#endif
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

#if 0
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

static void __init msm7627a_rumi3_init(void)
{
	msm7x27a_init_ebi2();
	platform_add_devices(rumi_sim_devices,
			ARRAY_SIZE(rumi_sim_devices));
}
#endif
#define LED_GPIO_PDM		17
#define UART1DM_RX_GPIO		45

#if defined(CONFIG_BT) && defined(CONFIG_MARIMBA_CORE)
static int __init msm7x27a_init_ar6000pm(void)
{
	return platform_device_register(&msm_wlan_ar6000_pm_device);
}
#else
static int __init msm7x27a_init_ar6000pm(void) { return 0; }
#endif

static void __init msm7x27a_init_regulators(void)
{
	int rc = platform_device_register(&msm_proccomm_regulator_dev);
	if (rc)
		pr_err("%s: could not register regulator device: %d\n",
				__func__, rc);
}

void golfu_add_usb_devices(void)
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

static void config_nfc_gpios(void)
{
	static uint32_t nfc_gpio_table[] = {
		GPIO_CFG(GOLFU_GPIO_NFC_VEN, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG(GOLFU_GPIO_NFC_DL, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG(GOLFU_GPIO_NFC_INT, 0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_DOWN, GPIO_CFG_8MA),
	};

	config_gpio_table(nfc_gpio_table,
		ARRAY_SIZE(nfc_gpio_table));
	pr_info("%s: EngineerID=%d\n", __func__, get_engineerid());
}

static struct pn544_i2c_platform_data nfc_platform_data = {
	.gpio_init	= config_nfc_gpios,
	.irq_gpio = GOLFU_GPIO_NFC_INT,
	.ven_gpio = GOLFU_GPIO_NFC_VEN,
	.firm_gpio = GOLFU_GPIO_NFC_DL,
	.ven_isinvert = 1,
	.boot_mode = 0,
};

static struct i2c_board_info pn544_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO(PN544_I2C_NAME, 0x50 >> 1),
		.platform_data = &nfc_platform_data,
		.irq = MSM_GPIO_TO_INT(GOLFU_GPIO_NFC_INT),
	},
};

static void golfu_reset(void)
{
	gpio_set_value(GOLFU_GPIO_PS_HOLD, 0);
}
#if 0
static void __init golfu_init(void)
{
	msm7x2x_misc_init();

	printk(KERN_INFO "golfu_init() revision = 0x%x\n", system_rev);
	printk(KERN_INFO "MSM_PMEM_MDP_BASE=0x%x MSM_PMEM_ADSP_BASE=0x%x MSM_RAM_CONSOLE_BASE=0x%x MSM_FB_BASE=0x%x\n",
		MSM_PMEM_MDP_BASE, MSM_PMEM_ADSP_BASE, MSM_RAM_CONSOLE_BASE, MSM_FB_BASE);
	/* Must set msm_hw_reset_hook before first proc comm */
	msm_hw_reset_hook = golfu_reset;

#ifdef CONFIG_PERFLOCK_BOOT_LOCK
	perflock_init(&holiday_perflock_data);
#endif

	/* Common functions for SURF/FFA/RUMI3 */
	msm_device_i2c_init();

	rc = golfu_init_mmc(system_rev);
	if (rc)
		printk(KERN_CRIT "%s: MMC init failure (%d)\n", __func__, rc);

#ifdef CONFIG_BT
	bt_export_bd_address();
#endif

#ifdef CONFIG_SERIAL_MSM_HS
	msm_uart_dm1_pdata.rx_wakeup_irq = gpio_to_irq(GOLFU_GPIO_BT_HOST_WAKE);
	msm_device_uart_dm1.name = "msm_serial_hs_brcm"; /* for brcm */
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
#endif

/*	headset_init();*/
	platform_add_devices(msm_footswitch_devices,
		msm_num_footswitch_devices);
	platform_add_devices(golfu_devices,
			ARRAY_SIZE(golfu_devices));

	msm_pm_set_platform_data(msm7x27a_pm_data,
				ARRAY_SIZE(msm7x27a_pm_data));

	golfu_init_panel();
#if defined(CONFIG_I2C) && defined(CONFIG_GPIO_SX150X)
	register_i2c_devices();
#endif
#if defined(CONFIG_BT) && defined(CONFIG_MARIMBA_CORE)
	bt_power_init();
#endif

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_tps65200_devices, ARRAY_SIZE(i2c_tps65200_devices));
#ifdef CONFIG_MSM_CAMERA
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
			i2c_camera_devices,
			ARRAY_SIZE(i2c_camera_devices));
#endif
#if 0
#if 0
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_bma250_devices, ARRAY_SIZE(i2c_bma250_devices));
#endif

	/* Disable loading because of no Cypress chip consider by pcbid */
	if (system_rev >= 0x80) {
		printk(KERN_INFO "No Cypress chip!\n");
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_touch_pvt_device, ARRAY_SIZE(i2c_touch_pvt_device));
	} else
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_touch_device, ARRAY_SIZE(i2c_touch_device));

#endif
	golfu_init_keypad();
	golfu_wifi_init();

#ifdef CONFIG_MSM_RPC_VIBRATOR
/*	msm_init_pmic_vibrator();*/
#endif
#ifdef CONFIG_USB_ANDROID
	golfu_add_usb_devices();
#endif
#ifdef CONFIG_MSM_HTC_DEBUG_INFO
	htc_debug_info_init();
#endif
#if defined(CONFIG_MSM_SERIAL_DEBUGGER)
	if (!opt_disable_uart3)
		msm_serial_debug_init(MSM_UART3_PHYS, INT_UART3,
				&msm_device_uart3.dev, 1,
				MSM_GPIO_TO_INT(GOLFU_GPIO_UART3_RX));
#endif
	/*7x25a kgsl initializations*/
	msm7x25a_kgsl_3d0_init();
}

#else
static void __init golfu_init(void)
{
	struct proc_dir_entry *entry = NULL;
	int rc = 0, i = 0;
	struct kobject *properties_kobj;

	msm7x2x_misc_init();

	printk(KERN_INFO "golfu_init() revision = 0x%x\n", system_rev);

#ifdef CONFIG_PERFLOCK
	perflock_init(&golfu_perflock_data);
#endif

	/* Must set msm_hw_reset_hook before first proc comm */
	msm_hw_reset_hook = golfu_reset;

	/* Initialize regulators first so that other devices can use them */
	msm7x27a_init_regulators();

	/* Common functions for SURF/FFA/RUMI3 */
	msm_device_i2c_init();
	msm7x27a_init_ebi2();
	msm7x27a_cfg_uart2dm_serial();
#ifdef CONFIG_SERIAL_MSM_HS
	msm_uart_dm1_pdata.rx_wakeup_irq = gpio_to_irq(UART1DM_RX_GPIO);
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
#endif

	msm_device_otg.dev.platform_data = &msm_otg_pdata;

	if (board_mfg_mode() == 1)
		htc_headset_mgr_config[0].adc_max = 65535;

	/*msm7x27a_cfg_smsc911x();*/
	platform_add_devices(msm_footswitch_devices,
			msm_num_footswitch_devices);
	platform_add_devices(golfu_devices,
			ARRAY_SIZE(golfu_devices));

#if defined(CONFIG_MSM_CAMERA) /* for 3M/5M sensor probed */
		if(system_rev >= 0x1) /* for XB board */
		  platform_add_devices(golfu_camera_5M_devices,ARRAY_SIZE(golfu_camera_5M_devices));
		else /* for XA board */
		  platform_add_devices(golfu_camera_3M_devices,ARRAY_SIZE(golfu_camera_3M_devices));
#endif

	/*Just init usb_id pin for accessory, accessory may not be used in golfu */
	config_golfu_usb_id_gpios(0);
	/*usb driver won't be loaded in MFG 58 station and gift mode*/
	if (!(board_mfg_mode() == 6 || board_mfg_mode() == 7))
		golfu_add_usb_devices();

	/* Ensure ar6000pm device is registered before MMC/SDC */
	msm7x27a_init_ar6000pm();

	golfu_wifi_init();

#ifdef CONFIG_MMC_MSM
	msm7x27a_init_mmc();
	entry = create_proc_read_entry("emmc", 0, NULL, emmc_partition_read_proc, NULL);
	if (!entry)
		printk(KERN_ERR"Create /proc/emmc failed!\n");
#endif


        entry = create_proc_read_entry("dying_processes", 0, NULL, dying_processors_read_proc, NULL);
        if (!entry)
                printk(KERN_ERR "Create /proc/dying_processes FAILED!\n");

	msm_pm_set_platform_data(msm7x27a_pm_data,
				ARRAY_SIZE(msm7x27a_pm_data));
	BUG_ON(msm_pm_boot_init(MSM_PM_BOOT_CONFIG_RESET_VECTOR, ioremap(0x0, PAGE_SIZE)));
	golfu_init_panel();
#if defined(CONFIG_I2C) && defined(CONFIG_GPIO_SX150X)
	register_i2c_devices();
#endif
#if defined(CONFIG_BT) && defined(CONFIG_MARIMBA_CORE)
	bt_export_bd_address();
	bt_power_init();
#endif
	if (machine_is_msm7625a_surf() || machine_is_msm7625a_ffa()) {
		atmel_ts_pdata.min_x = 0;
		atmel_ts_pdata.max_x = 480;
		atmel_ts_pdata.min_y = 0;
		atmel_ts_pdata.max_y = 320;
	}

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_tps65200_devices, ARRAY_SIZE(i2c_tps65200_devices));

#ifdef CONFIG_FLASHLIGHT_TPS61310
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				tps61310_i2c_info,
				ARRAY_SIZE(tps61310_i2c_info));
#endif

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
		atmel_ts_i2c_info,
		ARRAY_SIZE(atmel_ts_i2c_info));


	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_bma250_devices, ARRAY_SIZE(i2c_bma250_devices));
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_cm36282_devices, ARRAY_SIZE(i2c_cm36282_devices));
#ifdef CONFIG_CODEC_AIC3254
	aic3254_lowlevel_init();
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_aic3254_devices, ARRAY_SIZE(i2c_aic3254_devices));
#endif

#if defined(CONFIG_MSM_CAMERA)
	/* msm_camera_vreg_init(); // sync 2.6.38*/
	if(system_rev >= 0x1) /* for XB board */
		i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
		i2c_camera_5M_devices,
		ARRAY_SIZE(i2c_camera_5M_devices));
	else /* for XA board */
		i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
		i2c_camera_3M_devices,
			ARRAY_SIZE(i2c_camera_3M_devices));
#endif
#if 0
	platform_device_register(&kp_pdev);
#endif
	platform_device_register(&hs_pdev);

#if 0
	/* configure it as a pdm function*/
	if (gpio_tlmm_config(GPIO_CFG(LED_GPIO_PDM, 3,
				GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE))
		pr_err("%s: gpio_tlmm_config for %d failed\n",
			__func__, LED_GPIO_PDM);
	else
		platform_device_register(&led_pdev);
#endif

#if 0
#if defined(CONFIG_MSM_SERIAL_DEBUGGER)
	if (!opt_disable_uart3)
		msm_serial_debug_init(MSM_UART3_PHYS, INT_UART3,
				&msm_device_uart3.dev, 1,
				MSM_GPIO_TO_INT(GOLFU_GPIO_UART3_RX));
#endif
#endif
	/*7x25a kgsl initializations*/
	msm7x25a_kgsl_3d0_init();

	if (board_build_flag() == 1) {
		for (i = 0; i < ARRAY_SIZE(golfu_ts_synaptics_data);  i++)
			golfu_ts_synaptics_data[i].mfg_flag = 1;
	}

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				i2c_touch_device,
				ARRAY_SIZE(i2c_touch_device));

	if (get_engineerid() != 0) {
		/*Ignore NFC driver if engineer id is 0 (without NFC)*/
		nfc_platform_data.boot_mode = board_mfg_mode();
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				pn544_i2c_boardinfo,
				ARRAY_SIZE(pn544_i2c_boardinfo));
	}

	properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (properties_kobj)
		rc = sysfs_create_group(properties_kobj,
				&golfu_properties_attr_group);
	if (!properties_kobj || rc)
		pr_err("failed to create board_properties\n");


	golfu_init_keypad();
	msm_init_pmic_vibrator(3000);

	if (get_kernel_flag() & KERNEL_FLAG_PM_MONITOR) {
		htc_monitor_init();
		htc_PM_monitor_init();
	}
}
#endif

static void __init golfu_fixup(struct machine_desc *desc, struct tag *tags,
							char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 1;
	mi->bank[0].start = MSM_LINUX_BASE;
	mi->bank[0].size = MSM_LINUX_SIZE;
}

MACHINE_START(GOLFU, "golfu")
	.boot_params	= PHYS_OFFSET + 0x100,
	.fixup = golfu_fixup,
	.map_io		= msm_common_io_init,
	.reserve	= msm7x27a_reserve,
	.init_irq	= msm_init_irq,
	.init_machine	= golfu_init,
	.timer		= &msm_timer,
MACHINE_END
