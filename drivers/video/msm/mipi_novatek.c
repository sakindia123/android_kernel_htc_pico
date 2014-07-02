/* Copyright (c) 2010-2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifdef CONFIG_SPI_QUP
#include <linux/spi/spi.h>
#endif
#include <linux/leds.h>
#include <mach/panel_id.h>
#include <mach/debug_display.h>
#include <mach/htc_battery_common.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_novatek.h"
#include "mdp4.h"

extern int mipi_status;
#define DEFAULT_BRIGHTNESS 143
extern int bl_level_prevset;
extern struct dsi_cmd_desc *mipi_power_on_cmd;
extern struct dsi_cmd_desc *mipi_power_off_cmd;
extern int mipi_power_on_cmd_size;
extern int mipi_power_off_cmd_size;
extern char ptype[60];

static struct mipi_dsi_panel_platform_data *mipi_novatek_pdata;

static struct dsi_buf novatek_tx_buf;
static struct dsi_buf novatek_rx_buf;
static int mipi_novatek_lcd_init(void);

static int wled_trigger_initialized;

#define MIPI_DSI_NOVATEK_SPI_DEVICE_NAME	"dsi_novatek_3d_panel_spi"
#define HPCI_FPGA_READ_CMD	0x84
#define HPCI_FPGA_WRITE_CMD	0x04

#ifdef CONFIG_SPI_QUP
static struct spi_device *panel_3d_spi_client;

static void novatek_fpga_write(uint8 addr, uint16 value)
{
	char tx_buf[32];
	int  rc;
	struct spi_message  m;
	struct spi_transfer t;
	u8 data[4] = {0x0, 0x0, 0x0, 0x0};

	if (!panel_3d_spi_client) {
		pr_err("%s panel_3d_spi_client is NULL\n", __func__);
		return;
	}
	data[0] = HPCI_FPGA_WRITE_CMD;
	data[1] = addr;
	data[2] = ((value >> 8) & 0xFF);
	data[3] = (value & 0xFF);

	memset(&t, 0, sizeof t);
	memset(tx_buf, 0, sizeof tx_buf);
	t.tx_buf = data;
	t.len = 4;
	spi_setup(panel_3d_spi_client);
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	rc = spi_sync(panel_3d_spi_client, &m);
	if (rc)
		pr_err("%s: SPI transfer failed\n", __func__);

	return;
}

static void novatek_fpga_read(uint8 addr)
{
	char tx_buf[32];
	int  rc;
	struct spi_message  m;
	struct spi_transfer t;
	struct spi_transfer rx;
	char rx_value[2];
	u8 data[4] = {0x0, 0x0};

	if (!panel_3d_spi_client) {
		pr_err("%s panel_3d_spi_client is NULL\n", __func__);
		return;
	}

	data[0] = HPCI_FPGA_READ_CMD;
	data[1] = addr;

	memset(&t, 0, sizeof t);
	memset(tx_buf, 0, sizeof tx_buf);
	memset(&rx, 0, sizeof rx);
	memset(rx_value, 0, sizeof rx_value);
	t.tx_buf = data;
	t.len = 2;
	rx.rx_buf = rx_value;
	rx.len = 2;
	spi_setup(panel_3d_spi_client);
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	spi_message_add_tail(&rx, &m);

	rc = spi_sync(panel_3d_spi_client, &m);
	if (rc)
		pr_err("%s: SPI transfer failed\n", __func__);
	else
		pr_info("%s: rx_value = 0x%x, 0x%x\n", __func__,
						rx_value[0], rx_value[1]);

	return;
}

static int __devinit panel_3d_spi_probe(struct spi_device *spi)
{
	panel_3d_spi_client = spi;
	return 0;
}
static int __devexit panel_3d_spi_remove(struct spi_device *spi)
{
	panel_3d_spi_client = NULL;
	return 0;
}
static struct spi_driver panel_3d_spi_driver = {
	.probe         = panel_3d_spi_probe,
	.remove        = __devexit_p(panel_3d_spi_remove),
	.driver		   = {
		.name = "dsi_novatek_3d_panel_spi",
		.owner  = THIS_MODULE,
	}
};

#else

static void novatek_fpga_write(uint8 addr, uint16 value)
{
	return;
}

static void novatek_fpga_read(uint8 addr)
{
	return;
}

#endif


static unsigned char bkl_enable_cmds[] = {0x53, 0x24};/* DTYPE_DCS_WRITE1 */
static unsigned char bkl_disable_cmds[] = {0x53, 0x00};/* DTYPE_DCS_WRITE1 */


static char sw_reset[2] = {0x01, 0x00}; /* DTYPE_DCS_WRITE */
static char enter_sleep[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */
static char exit_sleep[2] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */
static char display_off[2] = {0x28, 0x00}; /* DTYPE_DCS_WRITE */
static char display_on[2] = {0x29, 0x00}; /* DTYPE_DCS_WRITE */


/* Pico panel initial setting */
static char pio_f0[] = {
	/* DTYPE_DCS_LWRITE */
	0xF0, 0xAA, 0x55, 0x52
};
static char pio_f6[] = {
	/* DTYPE_DCS_LWRITE */
	0xF6, 0x05, 0x70, 0x65
};
static char pio_b4[] = {
	/* DTYPE_DCS_LWRITE */
	0xB4, 0x03, 0x03, 0x03
};
static char pio_bf[] = {
	/* DTYPE_DCS_LWRITE */
	0xBF, 0x03, 0x08, 0x00
};
static char pio_b8[] = {
	/* DTYPE_DCS_LWRITE */
	0xB8, 0xEF, 0x02, 0xFF
};
static char pio_b0[] = {
	/* DTYPE_DCS_LWRITE */
	0xB0, 0x00, 0x7A, 0xFF
};
static char pio_b1[] = {
	/* DTYPE_DCS_LWRITE */
	0xB1, 0xB7, 0x01, 0x28
};
static char pio_b2[] = {
	/* DTYPE_DCS_LWRITE */
	0xB2, 0xB7, 0x01, 0x28
};
static char pio_b3[] = {
	/* DTYPE_DCS_LWRITE */
	0xB3, 0xB7, 0x01, 0x28
};
static char pio_c0[] = {
	/* DTYPE_DCS_LWRITE */
	0xC0, 0x78, 0x78, 0x00,
	0x00, 0x00, 0x00, 0x00
};
static char pio_c1[] = {
	/* DTYPE_DCS_LWRITE */
	0xC1, 0x46, 0x46, 0x46,
	0xF2, 0x02, 0x00, 0xFF
};
static char pio_c2[] = {
	/* DTYPE_DCS_LWRITE */
	0xC2, 0x01, 0x04, 0x01
};
static char pio_26[2] = {0x26, 0x10}; /* DTYPE_DCS_WRITE1 */
static char pio_e0[20] = {
	/* DTYPE_DCS_LWRITE */
	0xE0, 0x45, 0x48, 0x51, 0x5A, 0x17, 0x29, 0x5A, 0x3D, 0x1E,
	0x28, 0x7E, 0x19, 0x3E, 0x4C, 0x66, 0x99, 0x30, 0x71, 0xFF
};
static char pio_e1[20] = {
	/* DTYPE_DCS_LWRITE */
	0xE1, 0x45, 0x48, 0x51, 0x5A, 0x17, 0x29, 0x5A, 0x3D, 0x1E,
	0x28, 0x7E, 0x19, 0x3E, 0x4C, 0x66, 0x99, 0x30, 0x71, 0xFF
};
static char pio_e2[20] = {
	/* DTYPE_DCS_LWRITE */
	0xE2, 0x21, 0x27, 0x39, 0x49, 0x20, 0x34, 0x61, 0x42, 0x1D,
	0x27, 0x82, 0x1C, 0x46, 0x55, 0x64, 0x89, 0x2A, 0x71, 0xFF
};
static char pio_e3[20] = {
	/* DTYPE_DCS_LWRITE */
	0xE3, 0x21, 0x27, 0x39, 0x49, 0x20, 0x34, 0x61, 0x42, 0x1D,
	0x27, 0x82, 0x1C, 0x46, 0x55, 0x64, 0x89, 0x2A, 0x71, 0xFF
};
static char pio_e4[20] = {
	/* DTYPE_DCS_LWRITE */
	0xE4, 0x40, 0x52, 0x6A, 0x77, 0x20, 0x33, 0x60, 0x50, 0x1C,
	0x24, 0x86, 0x0D, 0x29, 0x35, 0xF4, 0xF0, 0x70, 0x7F, 0xFF
};
static char pio_e5[20] = {
	/* DTYPE_DCS_LWRITE */
	0xE5, 0x40, 0x52, 0x6A, 0x77, 0x20, 0x33, 0x60, 0x50, 0x1C,
	0x24, 0x86, 0x0D, 0x29, 0x35, 0xF4, 0xF0, 0x70, 0x7F, 0xFF
};
static char pio_pwm2[2] = {0x53, 0x24}; /* DTYPE_DCS_WRITE1 */
static char pio_cb[] = {
	/* DTYPE_DCS_LWRITE */
	0xCB, 0x80, 0x04, 0xFF
};
static char pio_fe[2] = {0xFE, 0x08};/* DTYPE_DCS_WRITE1 */
static char pio_enable_te[2] = {0x35, 0x00};/* DTYPE_DCS_WRITE */
static char peripheral_on[2] = {0x00, 0x00}; /* DTYPE_DCS_READ */

static struct dsi_cmd_desc pico_auo_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(sw_reset), sw_reset},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f0), pio_f0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_b4), pio_b4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_bf), pio_bf},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_b8), pio_b8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_b1), pio_b1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_b2), pio_b2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_b3), pio_b3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c0), pio_c0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c1), pio_c1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c2), pio_c2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_26), pio_26},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e0), pio_e0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e1), pio_e1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e2), pio_e2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e3), pio_e3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e4), pio_e4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e5), pio_e5},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_pwm2), pio_pwm2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_cb), pio_cb},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_enable_te), pio_enable_te},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 20,
		sizeof(pio_fe), pio_fe},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc pico_auo_video_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(sw_reset), sw_reset},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f0), pio_f0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f6), pio_f6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_b0), pio_b0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_b4), pio_b4},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_bf), pio_bf},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_b8), pio_b8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_b1), pio_b1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_b2), pio_b2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_b3), pio_b3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c0), pio_c0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c1), pio_c1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c2), pio_c2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_26), pio_26},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e0), pio_e0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e1), pio_e1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e2), pio_e2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e3), pio_e3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e4), pio_e4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e5), pio_e5},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_pwm2), pio_pwm2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_cb), pio_cb},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(pio_enable_te), pio_enable_te},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 20,
		sizeof(pio_fe), pio_fe},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(display_on), display_on},
	{DTYPE_PERIPHERAL_ON, 1, 0, 1, 0,
		sizeof(peripheral_on), peripheral_on}
};

static struct dsi_cmd_desc novatek_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc novatek_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(enter_sleep), enter_sleep}
};

static char manufacture_id[2] = {0x04, 0x00}; /* DTYPE_DCS_READ */

static struct dsi_cmd_desc novatek_manufacture_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id), manufacture_id};

static u32 manu_id;

static void mipi_novatek_manufacture_cb(u32 data)
{
	manu_id = data;
	pr_info("%s: manufacture_id=%x\n", __func__, manu_id);
}


/*
static struct dsi_cmd_desc novatek_manufacture_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id), manufacture_id};

static uint32 mipi_novatek_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	uint32 *lp;

	tp = &novatek_tx_buf;
	rp = &novatek_rx_buf;
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);

	cmd = &novatek_manufacture_id_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 3);
	lp = (uint32 *)rp->data;
	PR_DISP_INFO("%s: manufacture_id=%x\n", __func__, *lp);
	return *lp;
}
*/

static int fpga_addr;
static int fpga_access_mode;
static bool support_3d;

static void mipi_novatek_3d_init(int addr, int mode)
{
	fpga_addr = addr;
	fpga_access_mode = mode;
}

static void mipi_dsi_enable_3d_barrier(int mode)
{
	void __iomem *fpga_ptr;
	uint32_t ptr_value = 0;

	if (!fpga_addr && support_3d) {
		pr_err("%s: fpga_addr not set. Failed to enable 3D barrier\n",
					__func__);
		return;
	}

	if (fpga_access_mode == FPGA_SPI_INTF) {
		if (mode == LANDSCAPE)
			novatek_fpga_write(fpga_addr, 1);
		else if (mode == PORTRAIT)
			novatek_fpga_write(fpga_addr, 3);
		else
			novatek_fpga_write(fpga_addr, 0);

		mb();
		novatek_fpga_read(fpga_addr);
	} else if (fpga_access_mode == FPGA_EBI2_INTF) {
		fpga_ptr = ioremap_nocache(fpga_addr, sizeof(uint32_t));
		if (!fpga_ptr) {
			pr_err("%s: FPGA ioremap failed."
				"Failed to enable 3D barrier\n",
						__func__);
			return;
		}

		ptr_value = readl_relaxed(fpga_ptr);
		if (mode == LANDSCAPE)
			writel_relaxed(((0xFFFF0000 & ptr_value) | 1),
								fpga_ptr);
		else if (mode == PORTRAIT)
			writel_relaxed(((0xFFFF0000 & ptr_value) | 3),
								fpga_ptr);
		else
			writel_relaxed((0xFFFF0000 & ptr_value),
								fpga_ptr);

		mb();
		iounmap(fpga_ptr);
	} else
		pr_err("%s: 3D barrier not configured correctly\n",
					__func__);
}

static int mipi_novatek_lcd_on(struct platform_device *pdev)
{

	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	struct msm_panel_info *pinfo;
	struct dcs_cmd_req cmdreq;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	pinfo = &mfd->panel_info;

	mipi  = &mfd->panel_info.mipi;

			mipi_dsi_cmds_tx(&novatek_tx_buf, pico_auo_cmd_on_cmds,
				ARRAY_SIZE(pico_auo_cmd_on_cmds));

	return 0;
}

static int mipi_novatek_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct dcs_cmd_req cmdreq;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

			mipi_dsi_cmds_tx(&novatek_tx_buf, novatek_display_off_cmds,
				ARRAY_SIZE(novatek_display_off_cmds));

	return 0;
}

static int mipi_novatek_lcd_late_init(struct platform_device *pdev)
{
	return 0;
}

DEFINE_LED_TRIGGER(bkl_led_trigger);

static char led_pwm1[2] = {0x51, 0x0};	/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc backlight_cmd[] = {
	DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(led_pwm1), led_pwm1};


static void mipi_novatek_set_backlight(struct msm_fb_data_type *mfd)
{

	if (mipi_novatek_pdata &&
	    mipi_novatek_pdata->gpio_set_backlight) {
		mipi_novatek_pdata->gpio_set_backlight(mfd->bl_level);
		return;
	}

	if ((mipi_novatek_pdata->enable_wled_bl_ctrl)
	    && (wled_trigger_initialized)) {
		led_trigger_event(bkl_led_trigger, mfd->bl_level);
		return;
	}

	led_pwm1[1] = (unsigned char)mfd->bl_level;

			mipi_dsi_cmds_tx(&novatek_tx_buf, backlight_cmd,
				ARRAY_SIZE(backlight_cmd));
}

static int mipi_dsi_3d_barrier_sysfs_register(struct device *dev);
static int barrier_mode;

static int __devinit mipi_novatek_lcd_probe(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	struct platform_device *current_pdev;
	static struct mipi_dsi_phy_ctrl *phy_settings;
	static char dlane_swap;

	if (pdev->id == 0) {
		mipi_novatek_pdata = pdev->dev.platform_data;

		if (mipi_novatek_pdata
			&& mipi_novatek_pdata->phy_ctrl_settings) {
			phy_settings = (mipi_novatek_pdata->phy_ctrl_settings);
		}

		if (mipi_novatek_pdata
			 && mipi_novatek_pdata->fpga_3d_config_addr)
			mipi_novatek_3d_init(mipi_novatek_pdata
	->fpga_3d_config_addr, mipi_novatek_pdata->fpga_ctrl_mode);

		/* create sysfs to control 3D barrier for the Sharp panel */
		if (mipi_dsi_3d_barrier_sysfs_register(&pdev->dev)) {
			pr_err("%s: Failed to register 3d Barrier sysfs\n",
						__func__);
			return -ENODEV;
		}
		barrier_mode = 0;

		return 0;
	}


	current_pdev = msm_fb_add_device(pdev);

	if (current_pdev) {
		mfd = platform_get_drvdata(current_pdev);
		if (!mfd)
			return -ENODEV;
		if (mfd->key != MFD_KEY)
			return -EINVAL;

		mipi  = &mfd->panel_info.mipi;

		if (phy_settings != NULL)
			mipi->dsi_phy_db = phy_settings;
	}
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_novatek_lcd_probe,
	.driver = {
		.name   = "mipi_novatek",
	},
};

static struct msm_fb_panel_data novatek_panel_data = {
	.on		= mipi_novatek_lcd_on,
	.off		= mipi_novatek_lcd_off,
	.late_init	= mipi_novatek_lcd_late_init,
	.set_backlight = mipi_novatek_set_backlight,
};

static ssize_t mipi_dsi_3d_barrier_read(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return snprintf((char *)buf, sizeof(buf), "%u\n", barrier_mode);
}

static ssize_t mipi_dsi_3d_barrier_write(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	int ret = -1;
	u32 data = 0;

	if (sscanf((char *)buf, "%u", &data) != 1) {
		dev_err(dev, "%s\n", __func__);
		ret = -EINVAL;
	} else {
		barrier_mode = data;
		if (data == 1)
			mipi_dsi_enable_3d_barrier(LANDSCAPE);
		else if (data == 2)
			mipi_dsi_enable_3d_barrier(PORTRAIT);
		else
			mipi_dsi_enable_3d_barrier(0);
	}

	return count;
}

static struct device_attribute mipi_dsi_3d_barrier_attributes[] = {
	__ATTR(enable_3d_barrier, 0664, mipi_dsi_3d_barrier_read,
					 mipi_dsi_3d_barrier_write),
};

static int mipi_dsi_3d_barrier_sysfs_register(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mipi_dsi_3d_barrier_attributes); i++)
		if (device_create_file(dev, mipi_dsi_3d_barrier_attributes + i))
			goto error;

	return 0;

error:
	for (; i >= 0 ; i--)
		device_remove_file(dev, mipi_dsi_3d_barrier_attributes + i);
	pr_err("%s: Unable to create interface\n", __func__);

	return -ENODEV;
}

static int ch_used[3];

int mipi_novatek_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_novatek_lcd_init();
	if (ret) {
		pr_err("mipi_novatek_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_novatek", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	novatek_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &novatek_panel_data,
		sizeof(novatek_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int mipi_novatek_lcd_init(void)
{
#ifdef CONFIG_SPI_QUP
	int ret;
	ret = spi_register_driver(&panel_3d_spi_driver);

	if (ret) {
		pr_err("%s: spi register failed: rc=%d\n", __func__, ret);
		platform_driver_unregister(&this_driver);
	} else
		pr_info("%s: SUCCESS (SPI)\n", __func__);
#endif

	led_trigger_register_simple("bkl_trigger", &bkl_led_trigger);
	pr_info("%s: SUCCESS (WLED TRIGGER)\n", __func__);
	wled_trigger_initialized = 1;

	mipi_dsi_buf_alloc(&novatek_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&novatek_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

