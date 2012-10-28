/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
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
#include <mach/panel_id.h>
#include <mach/debug_display.h>
#include <mach/htc_battery_common.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_samsung.h"

/* -----------------------------------------------------------------------------
 *                         External routine declaration
 * ----------------------------------------------------------------------------- */
extern int mipi_status;
#define DEFAULT_BRIGHTNESS 83
extern int bl_level_prevset;
extern struct dsi_cmd_desc *mipi_power_on_cmd;
extern struct dsi_cmd_desc *mipi_power_off_cmd;
extern int mipi_power_on_cmd_size;
extern int mipi_power_off_cmd_size;
extern char ptype[60];

static struct msm_panel_common_pdata *mipi_samsung_pdata;

static struct dsi_buf samsung_tx_buf;
static struct dsi_buf samsung_rx_buf;


static char led_pwm1[] =
{
	0x51, 0xff,
};

/*static char led_pwm2[] =
{
	0x53, 0x24,
};

static char led_pwm3[] =
{
	0x55, 0x00,
};*/

static unsigned char bkl_enable_cmds[] = {0x53, 0x24};/* DTYPE_DCS_WRITE1 */
static unsigned char bkl_disable_cmds[] = {0x53, 0x00};/* DTYPE_DCS_WRITE1 */
static unsigned char bkl_cabc_cmds[] = {0x55, 0x00};/* DTYPE_DCS_WRITE1 */

static char sw_reset[2] = {0x01, 0x00}; /* DTYPE_DCS_WRITE */
static char enter_sleep[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */
static char exit_sleep[2] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */
static char display_off[2] = {0x28, 0x00}; /* DTYPE_DCS_WRITE */
static char display_on[2] = {0x29, 0x00}; /* DTYPE_DCS_WRITE */
/* sync from 8x60 */
static char enable_te[2] = {0x35, 0x01};/* DTYPE_DCS_WRITE1 */
/* sync from 8x60 */
static char rgb_888[2] = {0x3A, 0x77}; /* DTYPE_DCS_WRITE1 */


/* Pico panel initial setting */
static char pio_f0[] = {
	/* DTYPE_DCS_LWRITE */
	0xF0, 0x5A, 0x5A, 0x00
};
static char pio_f1[] = {
	/* DTYPE_DCS_LWRITE */
	0xF1, 0x5A, 0x5A, 0x00
};
static char pio_f2[] = {
	/* DTYPE_DCS_LWRITE */
	0xF2, 0x3C, 0x7E, 0x03,
	0x18, 0x18, 0x02, 0x10,
	0x00, 0x2F, 0x10, 0xC8,
	0x5D, 0x5D, 0x00, 0x00
};
static char pio_fc[] = {
	/* DTYPE_DCS_LWRITE */
	0xFC, 0xA5, 0xA5, 0x00
};
static char pio_fd[] = {
	/* DTYPE_DCS_LWRITE */
	0xFD, 0x00, 0x00, 0x00,
	0x02, 0x4F, 0x92, 0x41,
	0x54, 0x49, 0x0C, 0x4C,
	0x8C, 0x70, 0xBF, 0x15
};
static char pio_f4[] = {
	/* DTYPE_DCS_LWRITE */
	0xF4, 0x02, 0xAE, 0x43,
	0x43, 0x43, 0x43, 0x50,
	0x32, 0x22, 0x54, 0x51,
	0x20, 0x2A, 0x2A, 0x66
};
static char pio_C2_f4[] = {
	/* DTYPE_DCS_LWRITE */
	0xF4, 0x02, 0xAE, 0x43,
	0x43, 0x43, 0x43, 0x50,
	0x32, 0x13, 0x54, 0x51,
	0x20, 0x2A, 0x2A, 0x62
};
static char pio_f5[] = {
	/* DTYPE_DCS_LWRITE */
	0xF5, 0x4C, 0x75, 0x00
};
static char pio_C2_f5[] = {
	/* DTYPE_DCS_LWRITE */
	0xF5, 0x4A, 0x75, 0x00
};
static char pio_f6[] = {
	/* DTYPE_DCS_LWRITE */
	0xF6, 0x29, 0x02, 0x0F,
	0x00, 0x04, 0x77, 0x55,
	0x15, 0x00, 0x00, 0x00
};
static char pio_C2_f6[] = {
	/* DTYPE_DCS_LWRITE */
	0xF6, 0x2C, 0x02, 0x0F,
	0x00, 0x03, 0x22, 0x1D,
	0x00, 0x00, 0x00, 0x00
};
static char pio_f7[] = {
	/* DTYPE_DCS_LWRITE */
	0xF7, 0x01, 0x80, 0x12,
	0x02, 0x00, 0x00, 0x00
};
static char pio_f8[] = {
	/* DTYPE_DCS_LWRITE */
	0xF8, 0x55, 0x00, 0x14,
	0x09, 0x40, 0x00, 0x04,
	0x0A, 0x00, 0x00, 0x00
};
static char pio_C2_f8[] = {
	/* DTYPE_DCS_LWRITE */
	0xF8, 0x55, 0x00, 0x14,
	0x19, 0x40, 0x00, 0x04,
	0x0A, 0x00, 0x00, 0x00
};
static char pio_ed[2] = {0xED, 0x17}; /* DTYPE_DCS_WRITE1 */

static char pio_f9_1[2] = {0xF9, 0x04}; /* DTYPE_DCS_WRITE1 */
static char pio_fa_1[] = {
	/* DTYPE_DCS_LWRITE */
	0xFA, 0x09, 0x29, 0x04,
	0x15, 0x18, 0x0C, 0x0F,
	0x0E, 0x29, 0x22, 0x21,
	0x01, 0x0B, 0x00, 0x00
};
static char pio_C2_fa_1[] = {
	/* DTYPE_DCS_LWRITE */
	0xFA, 0x09, 0x29, 0x04,
	0x15, 0x18, 0x14, 0x15,
	0x10, 0x29, 0x22, 0x21,
	0x01, 0x0B, 0x00, 0x00
};
static char pio_fb_1[] = {
	/* DTYPE_DCS_LWRITE */
	0xFB, 0x03, 0x29, 0x00,
	0x0F, 0x16, 0x0E, 0x11,
	0x12, 0x25, 0x22, 0x21,
	0x01, 0x0B, 0x00, 0x00
};
static char pio_C2_fb_1[] = {
	/* DTYPE_DCS_LWRITE */
	0xFB, 0x03, 0x29, 0x00,
	0x0F, 0x16, 0x12, 0x14,
	0x12, 0x25, 0x22, 0x21,
	0x01, 0x0B, 0x00, 0x00
};
static char pio_f9_2[2] = {0xF9, 0x02}; /* DTYPE_DCS_WRITE1 */
static char pio_fa_2[] = {
	/* DTYPE_DCS_LWRITE */
	0xFA, 0x1A, 0x2B, 0x06,
	0x15, 0x17, 0x08, 0x0C,
	0x0A, 0x2C, 0x24, 0x1E,
	0x20, 0x20, 0x00, 0x00
};
static char pio_C2_fa_2[] = {
	/* DTYPE_DCS_LWRITE */
	0xFA, 0x1A, 0x2B, 0x06,
	0x15, 0x17, 0x0F, 0x0F,
	0x0A, 0x2A, 0x24, 0x1E,
	0x20, 0x20, 0x00, 0x00
};
static char pio_fb_2[] = {
	/* DTYPE_DCS_LWRITE */
	0xFB, 0x15, 0x2B, 0x00,
	0x0F, 0x15, 0x0A, 0x0E,
	0x0E, 0x28, 0x24, 0x1E,
	0x20, 0x20, 0x00, 0x00
};
static char pio_C2_fb_2[] = {
	/* DTYPE_DCS_LWRITE */
	0xFB, 0x15, 0x2B, 0x00,
	0x0F, 0x15, 0x0E, 0x0E,
	0x0E, 0x29, 0x24, 0x1E,
	0x20, 0x20, 0x00, 0x00
};
static char pio_f9_3[2] = {0xF9, 0x01}; /* DTYPE_DCS_WRITE1 */
static char pio_fa_3[] = {
	/* DTYPE_DCS_LWRITE */
	0xFA, 0x30, 0x2D, 0x08,
	0x14, 0x15, 0x00, 0x03,
	0x00, 0x35, 0x29, 0x08,
	0x00, 0x00, 0x00, 0x00
};
static char pio_C2_fa_3[] = {
	/* DTYPE_DCS_LWRITE */
	0xFA, 0x30, 0x2D, 0x08,
	0x14, 0x10, 0x00, 0x00,
	0x00, 0x35, 0x29, 0x08,
	0x00, 0x00, 0x00, 0x00
};
static char pio_fb_3[] = {
	/* DTYPE_DCS_LWRITE */
	0xFB, 0x2A, 0x2D, 0x00,
	0x0C, 0x11, 0x00, 0x05,
	0x06, 0x2F, 0x29, 0x08,
	0x00, 0x00, 0x00, 0x00
};
static char pio_C2_fb_3[] = {
	/* DTYPE_DCS_LWRITE */
	0xFB, 0x2A, 0x2D, 0x00,
	0x0C, 0x11, 0x00, 0x00,
	0x00, 0x38, 0x29, 0x08,
	0x00, 0x00, 0x00, 0x00
};
static char pio_c3[] = {
	/* DTYPE_DCS_LWRITE */
	0xC3, 0xD8, 0x00, 0x28
};
static char pio_c0[] = {
	/* DTYPE_DCS_LWRITE */
	0xC0, 0x80, 0x80, 0x00
};


static struct dsi_cmd_desc pico_samsung_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(sw_reset), sw_reset},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f0), pio_f0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f1), pio_f1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f2), pio_f2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f4), pio_f4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f5), pio_f5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f6), pio_f6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f7), pio_f7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f8), pio_f8},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_ed), pio_ed},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_f9_1), pio_f9_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_fa_1), pio_fa_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_fb_1), pio_fb_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_f9_2), pio_f9_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_fa_2), pio_fa_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_fb_2), pio_fb_2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_f9_3), pio_f9_3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_fa_3), pio_fa_3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_fb_3), pio_fb_3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c3), pio_c3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(bkl_cabc_cmds), bkl_cabc_cmds},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(rgb_888), rgb_888},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c0), pio_c0},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc pico_samsung_C2_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(sw_reset), sw_reset},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f0), pio_f0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f1), pio_f1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f2), pio_f2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_fc), pio_fc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_fd), pio_fd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_f4), pio_C2_f4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_f5), pio_C2_f5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_f6), pio_C2_f6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f7), pio_f7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_f8), pio_C2_f8},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_ed), pio_ed},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_f9_1), pio_f9_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_fa_1), pio_C2_fa_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_fb_1), pio_C2_fb_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_f9_2), pio_f9_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_fa_2), pio_C2_fa_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_fb_2), pio_C2_fb_2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_f9_3), pio_f9_3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_fa_3), pio_C2_fa_3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_fb_3), pio_C2_fb_3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c3), pio_c3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c0), pio_c0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(bkl_cabc_cmds), bkl_cabc_cmds},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(rgb_888), rgb_888},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(enable_te), enable_te},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};


static struct dsi_cmd_desc samsung_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(enter_sleep), enter_sleep}
};

/*
static struct dsi_cmd_desc samsung_manufacture_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id), manufacture_id};

static uint32 mipi_samsung_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	uint32 *lp;

	tp = &samsung_tx_buf;
	rp = &samsung_rx_buf;
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);

	cmd = &samsung_manufacture_id_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 3);
	lp = (uint32 *)rp->data;
	PR_DISP_INFO("%s: manufacture_id=%x\n", __func__, *lp);
	return *lp;
}
*/

static struct dsi_cmd_desc samsung_cmd_backlight_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm1), led_pwm1},
};

static struct dsi_cmd_desc samsung_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc samsung_bkl_enable_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(bkl_enable_cmds), bkl_enable_cmds},
};

static struct dsi_cmd_desc samsung_bkl_disable_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(bkl_disable_cmds), bkl_disable_cmds},
};

void mipi_samsung_panel_type_detect(struct mipi_panel_info *mipi)
{
	if (panel_type == PANEL_ID_PIO_SAMSUNG) {
		PR_DISP_INFO("%s: panel_type=PANEL_ID_PIO_SAMSUNG\n", __func__);
		strcat(ptype, "PANEL_ID_PIO_SAMSUNG");
		if (mipi->mode == DSI_VIDEO_MODE) {
			mipi_power_on_cmd = pico_samsung_cmd_on_cmds;
			mipi_power_on_cmd_size = ARRAY_SIZE(pico_samsung_cmd_on_cmds);
		} else {
			mipi_power_on_cmd = pico_samsung_cmd_on_cmds;
			mipi_power_on_cmd_size = ARRAY_SIZE(pico_samsung_cmd_on_cmds);
		}
		mipi_power_off_cmd = samsung_display_off_cmds;
		mipi_power_off_cmd_size = ARRAY_SIZE(samsung_display_off_cmds);
	} else if (panel_type == PANEL_ID_PIO_SAMSUNG_C2) {
		PR_DISP_INFO("%s: panel_type=PANEL_ID_PIO_SAMSUNG_C2\n", __func__);
		strcat(ptype, "PANEL_ID_PIO_SAMSUNG_C2");
		if (mipi->mode == DSI_VIDEO_MODE) {
			mipi_power_on_cmd = pico_samsung_C2_cmd_on_cmds;
			mipi_power_on_cmd_size = ARRAY_SIZE(pico_samsung_C2_cmd_on_cmds);
		} else {
			mipi_power_on_cmd = pico_samsung_C2_cmd_on_cmds;
			mipi_power_on_cmd_size = ARRAY_SIZE(pico_samsung_C2_cmd_on_cmds);
		}
		mipi_power_off_cmd = samsung_display_off_cmds;
		mipi_power_off_cmd_size = ARRAY_SIZE(samsung_display_off_cmds);
	} else {
		printk(KERN_ERR "%s: panel_type=0x%x not support\n", __func__, panel_type);
		strcat(ptype, "PANEL_ID_NONE");
	}
	return;
}


static int mipi_samsung_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct msm_fb_panel_data *pdata = NULL;
	struct mipi_panel_info *mipi;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	pdata = (struct msm_fb_panel_data *)mfd->pdev->dev.platform_data;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;

	if (mfd->init_mipi_lcd == 0) {
		PR_DISP_INFO("Display On - 1st time\n");

		if (pdata && pdata->panel_type_detect)
			pdata->panel_type_detect(mipi);

		mfd->init_mipi_lcd = 1;

	} else {
		PR_DISP_INFO("Display On \n");
		if (panel_type != PANEL_ID_NONE) {
			PR_DISP_INFO("%s\n", ptype);

			htc_mdp_sem_down(current, &mfd->dma->mutex);
			mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, mipi_power_on_cmd,
				mipi_power_on_cmd_size);
			htc_mdp_sem_up(&mfd->dma->mutex);
#if 0 /* mipi read command verify */
			/* clean up ack_err_status */
			mipi_dsi_cmd_bta_sw_trigger();
			mipi_samsung_manufacture_id(mfd);
#endif
		} else {
			printk(KERN_ERR "panel_type=0x%x not support at power on\n", panel_type);
			return -EINVAL;
		}
	}
	PR_DISP_DEBUG("Init done!\n");

	return 0;
}

static int mipi_samsung_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	PR_DISP_INFO("%s\n", __func__);

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (panel_type != PANEL_ID_NONE) {
		PR_DISP_INFO("%s\n", ptype);
		mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, mipi_power_off_cmd,
			mipi_power_off_cmd_size);
	} else
		printk(KERN_ERR "panel_type=0x%x not support at power off\n",
			panel_type);

	return 0;
}

static int mipi_dsi_set_backlight(struct msm_fb_data_type *mfd)
{
	struct mipi_panel_info *mipi;

	mipi  = &mfd->panel_info.mipi;
	if (mipi_status == 0)
		goto end;

	if (mipi_samsung_pdata && mipi_samsung_pdata->shrink_pwm)
		led_pwm1[1] = mipi_samsung_pdata->shrink_pwm(mfd->bl_level);
	else
		led_pwm1[1] = (unsigned char)(mfd->bl_level);

	if (mfd->bl_level == 0 || board_mfg_mode() == 4 ||
	    (board_mfg_mode() == 5 && !(htc_battery_get_zcharge_mode() % 2))) {
		led_pwm1[1] = 0;
	}

	htc_mdp_sem_down(current, &mfd->dma->mutex);
	if (mipi->mode == DSI_VIDEO_MODE) {
		mipi_dsi_cmd_mode_ctrl(1);	/* enable cmd mode */
		mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, samsung_cmd_backlight_cmds,
			ARRAY_SIZE(samsung_cmd_backlight_cmds));
		mipi_dsi_cmd_mode_ctrl(0);	/* disable cmd mode */
	} else {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
		mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, samsung_cmd_backlight_cmds,
			ARRAY_SIZE(samsung_cmd_backlight_cmds));
	}
	htc_mdp_sem_up(&mfd->dma->mutex);

	if (led_pwm1[1] != 0)
		bl_level_prevset = mfd->bl_level;

	PR_DISP_DEBUG("mipi_dsi_set_backlight > set brightness to %d\n", led_pwm1[1]);
end:
	return 0;
}

static void mipi_samsung_set_backlight(struct msm_fb_data_type *mfd)
{
	int bl_level;

	bl_level = mfd->bl_level;

	mipi_dsi_set_backlight(mfd);
}

static void mipi_samsung_display_on(struct msm_fb_data_type *mfd)
{
	PR_DISP_DEBUG("%s+\n", __func__);
	htc_mdp_sem_down(current, &mfd->dma->mutex);
	mipi_dsi_op_mode_config(DSI_CMD_MODE);
	mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, samsung_display_on_cmds,
		ARRAY_SIZE(samsung_display_on_cmds));
	htc_mdp_sem_up(&mfd->dma->mutex);
}

static void mipi_samsung_bkl_switch(struct msm_fb_data_type *mfd, bool on)
{
	unsigned int val = 0;

	if (on) {
		mipi_status = 1;
		val = mfd->bl_level;
		if (val == 0) {
			if (bl_level_prevset != 0) {
				val = bl_level_prevset;
				mfd->bl_level = val;
			} else {
				val = DEFAULT_BRIGHTNESS;
				mfd->bl_level = val;
			}
		}
		mipi_dsi_set_backlight(mfd);
	} else {
		mipi_status = 0;
	}
}

static void mipi_samsung_bkl_ctrl(struct msm_fb_data_type *mfd, bool on)
{
	PR_DISP_INFO("mipi_novatek_bkl_ctrl > on = %x\n", on);
	htc_mdp_sem_down(current, &mfd->dma->mutex);
	if (on) {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
		mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, samsung_bkl_enable_cmds,
			ARRAY_SIZE(samsung_bkl_enable_cmds));
	} else {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
		mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, samsung_bkl_disable_cmds,
			ARRAY_SIZE(samsung_bkl_disable_cmds));
	}
	htc_mdp_sem_up(&mfd->dma->mutex);
}

static int mipi_samsung_lcd_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		mipi_samsung_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_samsung_lcd_probe,
	.driver = {
		.name   = "mipi_samsung",
	},
};

static struct msm_fb_panel_data samsung_panel_data = {
	.on		= mipi_samsung_lcd_on,
	.off		= mipi_samsung_lcd_off,
	.set_backlight  = mipi_samsung_set_backlight,
	.display_on  = mipi_samsung_display_on,
	.bklswitch	= mipi_samsung_bkl_switch,
	.bklctrl	= mipi_samsung_bkl_ctrl,
	.panel_type_detect = mipi_samsung_panel_type_detect,
};

static int ch_used[3];

int mipi_samsung_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_samsung", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	samsung_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &samsung_panel_data,
		sizeof(samsung_panel_data));
	if (ret) {
		PR_DISP_ERR("%s: platform_device_add_data failed!\n",
			__func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		PR_DISP_ERR("%s: platform_device_register failed!\n",
			__func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_samsung_lcd_init(void)
{
	mipi_dsi_buf_alloc(&samsung_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&samsung_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_samsung_lcd_init);
