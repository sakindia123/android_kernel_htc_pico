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
#include "mipi_sony.h"

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

static struct msm_panel_common_pdata *mipi_sony_pdata;

static struct dsi_buf sony_tx_buf;
static struct dsi_buf sony_rx_buf;


static char led_pwm1[] = {0x51, 0xff};
static char bkl_enable_cmds[] = {0x53, 0x24};/* DTYPE_DCS_WRITE1 */
static char bkl_disable_cmds[] = {0x53, 0x00};/* DTYPE_DCS_WRITE1 */
static char bkl_cabc_cmds[] = {0x55, 0x00};/* DTYPE_DCS_WRITE1 */

static char sw_reset[2] = {0x01, 0x00}; /* DTYPE_DCS_WRITE */
static char enter_sleep[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */
static char exit_sleep[2] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */
static char display_off[2] = {0x28, 0x00}; /* DTYPE_DCS_WRITE */
static char display_on[2] = {0x29, 0x00}; /* DTYPE_DCS_WRITE */


/* PrimoDS panel initial setting */
static char pro_mipi_lane[] = {0xAE, 0x01}; /* DTYPE_DCS_WRITE1 */
static char pro_f3[] = {0xF3, 0xAA}; /* DTYPE_DCS_WRITE1 */
static char pro_c9[] = {0xC9, 0x01}; /* DTYPE_DCS_WRITE1 */
//static char pro_00[] = {0x00, 0x01}; /* DTYPE_DCS_WRITE1 */
//static char pro_7d[] = {0x7D, 0x01}; /* DTYPE_DCS_WRITE1 */
//static char pro_22[] = {0x22, 0x05}; /* DTYPE_DCS_WRITE1 */
//static char pro_7f[] = {0x7F, 0xAA}; /* DTYPE_DCS_WRITE1 */
static char pro_ff[] = {0xFF, 0xAA}; /* DTYPE_DCS_WRITE1 */

static struct dsi_cmd_desc primods_sony_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(sw_reset), sw_reset},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(pro_mipi_lane), pro_mipi_lane},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(pro_f3), pro_f3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(pro_c9), pro_c9},
/*	{DTYPE_DCS_WRITE1, 1, 0, 0, 1
		, sizeof(pro_00), pro_00}, */
/*	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(pro_7d), pro_7d}, */
/*	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(pro_22), pro_22}, */
/*	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(pro_7f), pro_7f}, */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(pro_ff), pro_ff},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 1,
		sizeof(display_on), display_on},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(bkl_cabc_cmds), bkl_cabc_cmds}
};

static struct dsi_cmd_desc sony_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(enter_sleep), enter_sleep}
};

/*
static struct dsi_cmd_desc sony_manufacture_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id), manufacture_id};

static uint32 mipi_sony_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	uint32 *lp;

	tp = &sony_tx_buf;
	rp = &sony_rx_buf;
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);

	cmd = &sony_manufacture_id_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 3);
	lp = (uint32 *)rp->data;
	PR_DISP_INFO("%s: manufacture_id=%x\n", __func__, *lp);
	return *lp;
}
*/

static struct dsi_cmd_desc sony_cmd_backlight_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm1), led_pwm1},
};

static struct dsi_cmd_desc sony_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sony_bkl_enable_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(bkl_enable_cmds), bkl_enable_cmds},
};

static struct dsi_cmd_desc sony_bkl_disable_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(bkl_disable_cmds), bkl_disable_cmds},
};

void mipi_sony_panel_type_detect(struct mipi_panel_info *mipi)
{
	if (panel_type == PANEL_ID_PRIMODS_SONY) {
		PR_DISP_INFO("%s: panel_type=PANEL_ID_PRIMODS_SONY\n", __func__);
		strcat(ptype, "PANEL_ID_PRIMODS_SONY");

		mipi_power_on_cmd = primods_sony_cmd_on_cmds;
		mipi_power_on_cmd_size = ARRAY_SIZE(primods_sony_cmd_on_cmds);

		mipi_power_off_cmd = sony_display_off_cmds;
		mipi_power_off_cmd_size = ARRAY_SIZE(sony_display_off_cmds);
	} else if (panel_type == PANEL_ID_PRIMODD_SONY) {
		PR_DISP_INFO("%s: panel_type=PANEL_ID_PRIMODD_SONY\n", __func__);
		strcat(ptype, "PANEL_ID_PRIMODD_SONY");

		mipi_power_on_cmd = primods_sony_cmd_on_cmds;
		mipi_power_on_cmd_size = ARRAY_SIZE(primods_sony_cmd_on_cmds);

		mipi_power_off_cmd = sony_display_off_cmds;
		mipi_power_off_cmd_size = ARRAY_SIZE(sony_display_off_cmds);
	} else {
		printk(KERN_ERR "%s: panel_type=0x%x not support\n", __func__, panel_type);
		strcat(ptype, "PANEL_ID_NONE");
	}
	return;
}


static int mipi_sony_lcd_on(struct platform_device *pdev)
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
			mipi_dsi_cmds_tx(mfd, &sony_tx_buf, mipi_power_on_cmd,
				mipi_power_on_cmd_size);
			htc_mdp_sem_up(&mfd->dma->mutex);
#if 0 /* mipi read command verify */
			/* clean up ack_err_status */
			mipi_dsi_cmd_bta_sw_trigger();
			mipi_sony_manufacture_id(mfd);
#endif
		} else {
			printk(KERN_ERR "panel_type=0x%x not support at power on\n", panel_type);
			return -EINVAL;
		}
	}
	PR_DISP_DEBUG("Init done!\n");

	return 0;
}

static int mipi_sony_lcd_off(struct platform_device *pdev)
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
		mipi_dsi_cmds_tx(mfd, &sony_tx_buf, mipi_power_off_cmd,
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

	if (mipi_sony_pdata && mipi_sony_pdata->shrink_pwm)
		led_pwm1[1] = mipi_sony_pdata->shrink_pwm(mfd->bl_level);
	else
		led_pwm1[1] = (unsigned char)(mfd->bl_level);

	if (mfd->bl_level == 0 || board_mfg_mode() == 4 ||
	    (board_mfg_mode() == 5 && !(htc_battery_get_zcharge_mode() % 2))) {
		led_pwm1[1] = 0;
	}

	htc_mdp_sem_down(current, &mfd->dma->mutex);
	if (mipi->mode == DSI_VIDEO_MODE) {
		mipi_dsi_cmd_mode_ctrl(1);	/* enable cmd mode */
		mipi_dsi_cmds_tx(mfd, &sony_tx_buf, sony_cmd_backlight_cmds,
			ARRAY_SIZE(sony_cmd_backlight_cmds));
		mipi_dsi_cmd_mode_ctrl(0);	/* disable cmd mode */
	} else {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
		mipi_dsi_cmds_tx(mfd, &sony_tx_buf, sony_cmd_backlight_cmds,
			ARRAY_SIZE(sony_cmd_backlight_cmds));
	}
	htc_mdp_sem_up(&mfd->dma->mutex);

	if (led_pwm1[1] != 0)
		bl_level_prevset = mfd->bl_level;

	PR_DISP_DEBUG("mipi_dsi_set_backlight > set brightness to %d\n", led_pwm1[1]);
end:
	return 0;
}

static void mipi_sony_set_backlight(struct msm_fb_data_type *mfd)
{
	int bl_level;

	bl_level = mfd->bl_level;

	mipi_dsi_set_backlight(mfd);
}

static void mipi_sony_display_on(struct msm_fb_data_type *mfd)
{
	PR_DISP_DEBUG("%s+\n", __func__);
	htc_mdp_sem_down(current, &mfd->dma->mutex);
	mipi_dsi_op_mode_config(DSI_CMD_MODE);
	mipi_dsi_cmds_tx(mfd, &sony_tx_buf, sony_display_on_cmds,
		ARRAY_SIZE(sony_display_on_cmds));
	htc_mdp_sem_up(&mfd->dma->mutex);
}

static void mipi_sony_bkl_switch(struct msm_fb_data_type *mfd, bool on)
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

static void mipi_sony_bkl_ctrl(struct msm_fb_data_type *mfd, bool on)
{
	PR_DISP_INFO("mipi_novatek_bkl_ctrl > on = %x\n", on);
	htc_mdp_sem_down(current, &mfd->dma->mutex);
	if (on) {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
		mipi_dsi_cmds_tx(mfd, &sony_tx_buf, sony_bkl_enable_cmds,
			ARRAY_SIZE(sony_bkl_enable_cmds));
	} else {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
		mipi_dsi_cmds_tx(mfd, &sony_tx_buf, sony_bkl_disable_cmds,
			ARRAY_SIZE(sony_bkl_disable_cmds));
	}
	htc_mdp_sem_up(&mfd->dma->mutex);
}

static int mipi_sony_lcd_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		mipi_sony_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_sony_lcd_probe,
	.driver = {
		.name   = "mipi_sony",
	},
};

static struct msm_fb_panel_data sony_panel_data = {
	.on		= mipi_sony_lcd_on,
	.off		= mipi_sony_lcd_off,
	.set_backlight  = mipi_sony_set_backlight,
	.display_on  = mipi_sony_display_on,
	.bklswitch	= mipi_sony_bkl_switch,
	.bklctrl	= mipi_sony_bkl_ctrl,
	.panel_type_detect = mipi_sony_panel_type_detect,
};

static int ch_used[3];

int mipi_sony_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_sony", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	sony_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &sony_panel_data,
		sizeof(sony_panel_data));
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

static int __init mipi_sony_lcd_init(void)
{
	mipi_dsi_buf_alloc(&sony_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&sony_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_sony_lcd_init);
