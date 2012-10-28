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
#include "mipi_lg.h"

/* -----------------------------------------------------------------------------
 *                         External routine declaration
 * ----------------------------------------------------------------------------- */
extern int mipi_status;
#define DEFAULT_BRIGHTNESS 143
extern int bl_level_prevset;
extern struct dsi_cmd_desc *mipi_power_on_cmd;
extern struct dsi_cmd_desc *mipi_power_off_cmd;
extern int mipi_power_on_cmd_size;
extern int mipi_power_off_cmd_size;
extern char ptype[60];

static struct msm_panel_common_pdata *mipi_lg_pdata;

static struct dsi_buf lg_tx_buf;
static struct dsi_buf lg_rx_buf;

static unsigned int val_pre = DEFAULT_BRIGHTNESS;

static unsigned char write_control_display_on[] = {0x53, 0x24};/* DTYPE_DCS_WRITE1 */
static unsigned char write_control_display_off[] = {0x53, 0x00};/* DTYPE_DCS_WRITE1 */

static char display_off[2] = {0x28, 0x00}; /* DTYPE_DCS_WRITE */
static char sleep_in[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */
//static char deep_standby_mode_in[] = {0xC1, 0x01}; /* DTYPE_DCS_WRITE1 */
//static char sw_reset[2] = {0x01, 0x00}; /* DTYPE_DCS_WRITE */

/* PrimoDS panel initial setting */
static char display_inv[] = {0x20, 0x00}; /* DTYPE_DCS_WRITE */

static char set_address_mode[] = {0x36, 0x00}; /* DTYPE_DCS_WRITE1 */
static char interface_pixel_format[] = {0x3A, 0x70}; /* DTYPE_DCS_WRITE1 */

static char panel_characteristics_setting[] = {
	0xB2, 0x20, 0xC8}; /* DTYPE_DCS_LWRITE */
static char panel_drive_setting[] = {0xB3, 0x00}; /* DTYPE_DCS_WRITE1 */
static char display_mode_control[] = {0xB4, 0x04}; /* DTYPE_DCS_WRITE1 */
static char display_control1[] = {0xB5, 0x20, 0x10, 0x10}; /* DTYPE_DCS_WRITE1 */
static char display_control2[] = {
	0xB6, 0x03, 0x0F, 0x02, 0x40, 0x10, 0xE8}; /* DTYPE_DCS_LWRITE */
static char internal_oscillator_setting[] = {
	0xC0, 0x01, 0x18}; /* DTYPE_DCS_LWRITE */
static char power_control1[] = {0xC1, 0x08}; /* DTYPE_DCS_WRITE1 */
static char power_control2[] = {0xC2, 0x00}; /* DTYPE_DCS_WRITE1 */
static char power_control3[] = {
	0xC3, 0x07, 0x05, 0x05, 0x05, 0x07}; /* DTYPE_DCS_LWRITE */
static char power_control4[] = {
	0xC4, 0x12, 0x24, 0x12, 0x12, 0x05, 0x4c}; /* DTYPE_DCS_LWRITE */
static char mtp_vocm[] = {0xF9, 0x40}; /* DTYPE_DCS_WRITE1 */
static char power_control6[] = {
	0xC6, 0x41, 0x63, 0x03}; /* DTYPE_DCS_LWRITE */

static char positive_gamma_red[] = {
	0xD0, 0x03, 0x10, 0x73, 0x07, 0x00, 0x01, 0x50,
	0x13, 0x02}; /* DTYPE_DCS_LWRITE */

static char negative_gamma_red[] = {
	0xD1, 0x03, 0x10, 0x73, 0x07, 0x00, 0x02, 0x50,
	0x13, 0x02}; /* DTYPE_DCS_LWRITE */

static char positive_gamma_green[] = {
	0xD2, 0x03, 0x10, 0x73, 0x07, 0x00, 0x01, 0x50,
	0x13, 0x02}; /* DTYPE_DCS_LWRITE */

static char negative_gamma_green[] = {
	0xD3, 0x03, 0x10, 0x73, 0x07, 0x00, 0x02, 0x50,
	0x13, 0x02}; /* DTYPE_DCS_LWRITE */

static char positive_gamma_blue[] = {
	0xD4, 0x03, 0x10, 0x73, 0x07, 0x00, 0x01, 0x50,
	0x13, 0x02}; /* DTYPE_DCS_LWRITE */

static char negative_gamma_blue[] = {
	0xD5, 0x03, 0x10, 0x73, 0x07, 0x00, 0x02, 0x50,
	0x13, 0x02}; /* DTYPE_DCS_LWRITE */

static char sleep_out[] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */
static char display_on[] = {0x29, 0x00}; /* DTYPE_DCS_WRITE */

static char disable_high_speed_timeout[] = {0x03, 0x00}; /* DTYPE_GEN_WRITE2 */
static char backlight_control[] = {
	0xC8, 0x11, 0x03}; /* DTYPE_DCS_LWRITE */
static char write_display_brightness[] = {0x51, 0xFF}; /* DTYPE_DCS_WRITE1 */
static char write_content_adaptive_brightness_control[] = {0x55, 0x01}; /* DTYPE_DCS_WRITE1 */
static char write_cabc_minimum_brightness[] = {0x5E, 0xB3}; /* DTYPE_DCS_WRITE1 */

static struct dsi_cmd_desc lg_video_on_cmds[] = {
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0,
		sizeof(disable_high_speed_timeout), disable_high_speed_timeout},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_inv), display_inv},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(set_address_mode), set_address_mode},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(interface_pixel_format), interface_pixel_format},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(panel_characteristics_setting), panel_characteristics_setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(panel_drive_setting), panel_drive_setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(display_mode_control), display_mode_control},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(display_control1), display_control1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(display_control2), display_control2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(internal_oscillator_setting), internal_oscillator_setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(power_control1), power_control1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(power_control2), power_control2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(power_control3), power_control3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(power_control4), power_control4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(mtp_vocm), mtp_vocm},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(power_control6), power_control6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(positive_gamma_red), positive_gamma_red},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(negative_gamma_red), negative_gamma_red},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(positive_gamma_green), positive_gamma_green},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(negative_gamma_green), negative_gamma_green},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(positive_gamma_blue), positive_gamma_blue},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(negative_gamma_blue), negative_gamma_blue},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(write_cabc_minimum_brightness), write_cabc_minimum_brightness},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(write_content_adaptive_brightness_control), write_content_adaptive_brightness_control},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(backlight_control), backlight_control},
	{DTYPE_DCS_WRITE, 1, 0, 0, 100,
		sizeof(sleep_out), sleep_out},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc lg_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 20,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(sleep_in), sleep_in},
//	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
//		sizeof(deep_standby_mode_in), deep_standby_mode_in}
};

/* we don't need read manufacture id*/
#if 0
//static char manufacture_id[2] = {0x04, 0x00}; /* DTYPE_DCS_READ */

static struct dsi_cmd_desc lg_manufacture_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id), manufacture_id};

static uint32 mipi_lg_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	uint32 *lp;

	tp = &lg_tx_buf;
	rp = &lg_rx_buf;
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);

	cmd = &lg_manufacture_id_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 3);
	lp = (uint32 *)rp->data;
	PR_DISP_INFO("%s: manufacture_id=%x\n", __func__, *lp);

	return *lp;
}
#endif

static struct dsi_cmd_desc lg_cmd_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(write_display_brightness), write_display_brightness},
};

static struct dsi_cmd_desc lg_bkl_enable_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(backlight_control), backlight_control},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(write_control_display_on), write_control_display_on},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(write_content_adaptive_brightness_control), write_content_adaptive_brightness_control},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(write_cabc_minimum_brightness), write_cabc_minimum_brightness},
};

static struct dsi_cmd_desc lg_bkl_disable_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(write_control_display_off), write_control_display_off},
};

#if 0
static struct dsi_cmd_desc lg_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};
#endif

void mipi_lg_panel_type_detect(struct mipi_panel_info *mipi)
{
	if (panel_type == PANEL_ID_PRIMODS_LG) {
		PR_DISP_INFO("%s: panel_type=PANEL_ID_PRIMODS_lg\n", __func__);
		strcat(ptype, "PANEL_ID_PRIMODS_LG");

		mipi_power_on_cmd = lg_video_on_cmds;
		mipi_power_on_cmd_size = ARRAY_SIZE(lg_video_on_cmds);
		mipi_power_off_cmd = lg_display_off_cmds;
		mipi_power_off_cmd_size = ARRAY_SIZE(lg_display_off_cmds);
	} else if (panel_type == PANEL_ID_PRIMODD_LG) {
		PR_DISP_INFO("%s: panel_type=PANEL_ID_PRIMODD_lg\n", __func__);
		strcat(ptype, "PANEL_ID_PRIMODD_LG");

		mipi_power_on_cmd = lg_video_on_cmds;
		mipi_power_on_cmd_size = ARRAY_SIZE(lg_video_on_cmds);
		mipi_power_off_cmd = lg_display_off_cmds;
		mipi_power_off_cmd_size = ARRAY_SIZE(lg_display_off_cmds);
	} else {
		printk(KERN_ERR "%s: panel_type=0x%x not support\n", __func__, panel_type);
		strcat(ptype, "PANEL_ID_NONE");
	}
	return;
}


static int mipi_lg_lcd_on(struct platform_device *pdev)
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
			mipi_dsi_cmds_tx(mfd, &lg_tx_buf, mipi_power_on_cmd,
				mipi_power_on_cmd_size);
			htc_mdp_sem_up(&mfd->dma->mutex);
#if 0 /* mipi read command verify */
			/* clean up ack_err_status */
			mdelay(1000);
			mipi_dsi_cmd_bta_sw_trigger();
			mipi_lg_manufacture_id(mfd);
#endif
		} else {
			printk(KERN_ERR "panel_type=0x%x not support at power on\n", panel_type);
			return -EINVAL;
		}
	}
	PR_DISP_DEBUG("Init done!\n");

	return 0;
}

static int mipi_lg_lcd_off(struct platform_device *pdev)
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
		mipi_dsi_cmds_tx(mfd, &lg_tx_buf, mipi_power_off_cmd,
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
	if (mipi_status == 0 || bl_level_prevset == mfd->bl_level) {
		PR_DISP_DEBUG("Skip the backlight setting > mipi_status : %d, bl_level_prevset : %d, bl_level : %d\n",
			mipi_status, bl_level_prevset, mfd->bl_level);
		goto end;
	}

	if (mipi_lg_pdata && mipi_lg_pdata->shrink_pwm)
		write_display_brightness[1] = mipi_lg_pdata->shrink_pwm(mfd->bl_level);
	else
		write_display_brightness[1] = (unsigned char)(mfd->bl_level);

	if (mfd->bl_level == 0 || board_mfg_mode() == 4 ||
	    (board_mfg_mode() == 5 && !(htc_battery_get_zcharge_mode() % 2))) {
		write_display_brightness[1] = 0;
	}

	htc_mdp_sem_down(current, &mfd->dma->mutex);
	if (mipi->mode == DSI_VIDEO_MODE) {
		if (panel_type == PANEL_ID_PRIMODS_LG || panel_type == PANEL_ID_PRIMODD_LG) {
			/* This is added for LG panel which is needed to use BTA to clear the error happened in driverIC */
			MIPI_OUTP(MIPI_DSI_BASE + 0xA8, 0x10000000);
			mipi_dsi_cmd_bta_sw_trigger();

			/* Need to send blk disable cmd to turn off backlight, or it will change to dim brightness even sending 0 brightness */
			if (write_display_brightness[1] == 0)
				mipi_dsi_cmds_tx(mfd, &lg_tx_buf, lg_bkl_disable_cmds,
					ARRAY_SIZE(lg_bkl_disable_cmds));
			else if (bl_level_prevset == 0)
				mipi_dsi_cmds_tx(mfd, &lg_tx_buf, lg_bkl_enable_cmds,
					ARRAY_SIZE(lg_bkl_enable_cmds));
		}
		mipi_dsi_cmds_tx(mfd, &lg_tx_buf, lg_cmd_backlight_cmds,
			ARRAY_SIZE(lg_cmd_backlight_cmds));
	} else {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
		mipi_dsi_cmds_tx(mfd, &lg_tx_buf, lg_cmd_backlight_cmds,
			ARRAY_SIZE(lg_cmd_backlight_cmds));
	}
	htc_mdp_sem_up(&mfd->dma->mutex);

	bl_level_prevset = mfd->bl_level;

	/* Record the last value which was not zero for resume use */
	if (mfd->bl_level != 0)
		val_pre = mfd->bl_level;

	PR_DISP_DEBUG("mipi_dsi_set_backlight > set brightness to %d\n", write_display_brightness[1]);
end:
	return 0;
}

static void mipi_lg_set_backlight(struct msm_fb_data_type *mfd)
{
	if (!mfd->panel_power_on)
		return;

	mipi_dsi_set_backlight(mfd);
}


static void mipi_lg_display_on(struct msm_fb_data_type *mfd)
{
	PR_DISP_DEBUG("%s+\n", __func__);
#if 0 /* Skip the display_on cmd transfer for LG panel only */
	htc_mdp_sem_down(current, &mfd->dma->mutex);
	if (mfd->panel_info.type == MIPI_CMD_PANEL) {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
	}
	mipi_dsi_cmds_tx(mfd, &lg_tx_buf, lg_display_on_cmds,
		ARRAY_SIZE(lg_display_on_cmds));
	htc_mdp_sem_up(&mfd->dma->mutex);
#endif
}

static void mipi_lg_bkl_switch(struct msm_fb_data_type *mfd, bool on)
{
	PR_DISP_DEBUG("%s > %d\n", __func__, on);
	if (on) {
		mipi_status = 1;
		if (mfd->bl_level == 0)
			/* Assign the backlight by last brightness before suspend */
			mfd->bl_level = val_pre;

		mipi_dsi_set_backlight(mfd);
	} else {
		if (bl_level_prevset != 0) {
			val_pre = bl_level_prevset;
			mfd->bl_level = 0;
			mipi_dsi_set_backlight(mfd);
		}
		mipi_status = 0;
	}
}

#if 0
static void mipi_lg_bkl_ctrl(struct msm_fb_data_type *mfd, bool on)
{
	PR_DISP_INFO("mipi_lg_bkl_ctrl > on = %x\n", on);
	htc_mdp_sem_down(current, &mfd->dma->mutex);
	if (on) {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
		mipi_dsi_cmds_tx(mfd, &lg_tx_buf, lg_bkl_enable_cmds,
			ARRAY_SIZE(lg_bkl_enable_cmds));
	} else {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
		mipi_dsi_cmds_tx(mfd, &lg_tx_buf, lg_bkl_disable_cmds,
			ARRAY_SIZE(lg_bkl_disable_cmds));
	}
	htc_mdp_sem_up(&mfd->dma->mutex);
}
#endif

static int mipi_lg_lcd_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		mipi_lg_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_lg_lcd_probe,
	.driver = {
		.name   = "mipi_lg",
	},
};

static struct msm_fb_panel_data lg_panel_data = {
	.on		= mipi_lg_lcd_on,
	.off		= mipi_lg_lcd_off,
	.set_backlight  = mipi_lg_set_backlight,
	.display_on  = mipi_lg_display_on,
	.bklswitch	= mipi_lg_bkl_switch,
//	.bklctrl	= mipi_lg_bkl_ctrl,
	.panel_type_detect = mipi_lg_panel_type_detect,
};

static int ch_used[3];

int mipi_lg_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_lg", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	lg_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &lg_panel_data,
		sizeof(lg_panel_data));
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

static int __init mipi_lg_lcd_init(void)
{
	mipi_dsi_buf_alloc(&lg_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&lg_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_lg_lcd_init);
