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
#include "mipi_novatek.h"
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

static struct msm_panel_common_pdata *mipi_novatek_pdata;

static struct dsi_buf novatek_tx_buf;
static struct dsi_buf novatek_rx_buf;


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


static struct dsi_cmd_desc novatek_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(enter_sleep), enter_sleep}
};

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
static bool support_3d;

static void mipi_novatek_3d_init(int addr)
{
	fpga_addr = addr;
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

	fpga_ptr = ioremap_nocache(fpga_addr, sizeof(uint32_t));
	if (!fpga_ptr) {
		pr_err("%s: FPGA ioremap failed. Failed to enable 3D barrier\n",
					__func__);
		return;
	}

	ptr_value = readl_relaxed(fpga_ptr);
	if (mode == LANDSCAPE)
		writel_relaxed(((0xFFFF0000 & ptr_value) | 1), fpga_ptr);
	else if (mode == PORTRAIT)
		writel_relaxed(((0xFFFF0000 & ptr_value) | 3), fpga_ptr);
	else
		writel_relaxed((0xFFFF0000 & ptr_value), fpga_ptr);

	mb();
	iounmap(fpga_ptr);
}


static struct dsi_cmd_desc novatek_cmd_backlight_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm1), led_pwm1},
};

static struct dsi_cmd_desc novatek_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc novatek_bkl_enable_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(bkl_enable_cmds), bkl_enable_cmds},
};

static struct dsi_cmd_desc novatek_bkl_disable_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(bkl_disable_cmds), bkl_disable_cmds},
};

#ifdef NOVATEK_ESD_WORKAROUND
static char peripheral_off[2] = {0x00, 0x00};/* DTYPE_DCS_READ */
static struct dsi_cmd_desc prevent_esd_cmds[] = {
	{DTYPE_PERIPHERAL_OFF, 1, 0, 1, 0,
		sizeof(peripheral_off), peripheral_off},
};

/* EMC workaround for LCM hang after ESD test */
void mipi_novatek_set_prevent_esd(struct msm_fb_data_type *mfd)
{
	htc_mdp_sem_down(current, &mfd->dma->mutex);
	mipi_dsi_op_mode_config(DSI_CMD_MODE);
	mipi_dsi_cmds_tx(mfd, &novatek_tx_buf, prevent_esd_cmds,
		ARRAY_SIZE(prevent_esd_cmds));
	htc_mdp_sem_up(&mfd->dma->mutex);
}
/* EMC workaround for LCM hang after ESD test */
#endif

void mipi_novatek_panel_type_detect(struct mipi_panel_info *mipi)
{
	if (panel_type == PANEL_ID_PIO_AUO) {
		PR_DISP_INFO("%s: panel_type=PANEL_ID_PIO_AUO\n", __func__);
		strcat(ptype, "PANEL_ID_PIO_AUO");
		if (mipi->mode == DSI_VIDEO_MODE) {
			mipi_power_on_cmd = pico_auo_video_on_cmds;
			mipi_power_on_cmd_size = ARRAY_SIZE(pico_auo_video_on_cmds);
		} else {
			mipi_power_on_cmd = pico_auo_cmd_on_cmds;
			mipi_power_on_cmd_size = ARRAY_SIZE(pico_auo_cmd_on_cmds);
		}
		mipi_power_off_cmd = novatek_display_off_cmds;
		mipi_power_off_cmd_size = ARRAY_SIZE(novatek_display_off_cmds);
	} else {
		printk(KERN_ERR "%s: panel_type=0x%x not support\n", __func__, panel_type);
		strcat(ptype, "PANEL_ID_NONE");
	}
	return;
}


static int mipi_dsi_set_backlight(struct msm_fb_data_type *mfd)
{
	struct mipi_panel_info *mipi;

	mipi  = &mfd->panel_info.mipi;
	if (mipi_status == 0)
		goto end;

	if (mipi_novatek_pdata && mipi_novatek_pdata->shrink_pwm)
		led_pwm1[1] = mipi_novatek_pdata->shrink_pwm(mfd->bl_level);
	else
		led_pwm1[1] = (unsigned char)(mfd->bl_level);

	if (mfd->bl_level == 0 || board_mfg_mode() == 4 ||
	    (board_mfg_mode() == 5 && !(htc_battery_get_zcharge_mode() % 2))) {
		led_pwm1[1] = 0;
	}

	htc_mdp_sem_down(current, &mfd->dma->mutex);
	if (mipi->mode == DSI_VIDEO_MODE) {
		mipi_dsi_cmd_mode_ctrl(1);	/* enable cmd mode */
		mipi_dsi_cmds_tx(mfd, &novatek_tx_buf, novatek_cmd_backlight_cmds,
			ARRAY_SIZE(novatek_cmd_backlight_cmds));
		mipi_dsi_cmd_mode_ctrl(0);	/* disable cmd mode */
	} else {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
		mipi_dsi_cmds_tx(mfd, &novatek_tx_buf, novatek_cmd_backlight_cmds,
			ARRAY_SIZE(novatek_cmd_backlight_cmds));
	}
	htc_mdp_sem_up(&mfd->dma->mutex);

	if (led_pwm1[1] != 0)
		bl_level_prevset = mfd->bl_level;

	PR_DISP_DEBUG("mipi_dsi_set_backlight > set brightness to %d\n", led_pwm1[1]);
end:
	return 0;
}

static void mipi_novatek_set_backlight(struct msm_fb_data_type *mfd)
{
	int bl_level;

	bl_level = mfd->bl_level;

	mipi_dsi_set_backlight(mfd);

}

static void mipi_novatek_display_on(struct msm_fb_data_type *mfd)
{
	PR_DISP_DEBUG("%s+\n", __func__);
	htc_mdp_sem_down(current, &mfd->dma->mutex);
	mipi_dsi_op_mode_config(DSI_CMD_MODE);
	mipi_dsi_cmds_tx(mfd, &novatek_tx_buf, novatek_display_on_cmds,
		ARRAY_SIZE(novatek_display_on_cmds));
	htc_mdp_sem_up(&mfd->dma->mutex);
}

static void mipi_novatek_bkl_switch(struct msm_fb_data_type *mfd, bool on)
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

static void mipi_novatek_bkl_ctrl(struct msm_fb_data_type *mfd, bool on)
{
	PR_DISP_DEBUG("mipi_novatek_bkl_ctrl > on = %x\n", on);
	htc_mdp_sem_down(current, &mfd->dma->mutex);
	if (on) {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
		mipi_dsi_cmds_tx(mfd, &novatek_tx_buf, novatek_bkl_enable_cmds,
			ARRAY_SIZE(novatek_bkl_enable_cmds));
	} else {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
		mipi_dsi_cmds_tx(mfd, &novatek_tx_buf, novatek_bkl_disable_cmds,
			ARRAY_SIZE(novatek_bkl_disable_cmds));
	}
	htc_mdp_sem_up(&mfd->dma->mutex);
}

static int mipi_novatek_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct msm_fb_panel_data *pdata = NULL;
	struct msm_panel_info *pinfo;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	pdata = (struct msm_fb_panel_data *)mfd->pdev->dev.platform_data;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	pinfo = &mfd->panel_info;
	if (pinfo->is_3d_panel)
		support_3d = TRUE;

	if (mfd->init_mipi_lcd == 0) {
		PR_DISP_DEBUG("Display On - 1st time\n");

		if (pdata && pdata->panel_type_detect)
			pdata->panel_type_detect(&pinfo->mipi);

		mfd->init_mipi_lcd = 1;

	} else {
		PR_DISP_DEBUG("Display On \n");

		if (panel_type != PANEL_ID_NONE) {
			PR_DISP_INFO("%s\n", ptype);

			htc_mdp_sem_down(current, &mfd->dma->mutex);
			mipi_dsi_cmds_tx(mfd, &novatek_tx_buf, mipi_power_on_cmd,
				mipi_power_on_cmd_size);
			htc_mdp_sem_up(&mfd->dma->mutex);
#if 0 /* mipi read command verify */
				/* clean up ack_err_status */
				mipi_dsi_cmd_bta_sw_trigger();
				mipi_novatek_manufacture_id(mfd);
#endif
		} else {
			printk(KERN_ERR "panel_type=0x%x not support at power on\n", panel_type);
			return -EINVAL;
		}
	}
	PR_DISP_DEBUG("Init done!\n");

	return 0;
}

static int mipi_novatek_lcd_off(struct platform_device *pdev)
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
		mipi_dsi_cmds_tx(mfd, &novatek_tx_buf, mipi_power_off_cmd,
			mipi_power_off_cmd_size);
	} else
		printk(KERN_ERR "panel_type=0x%x not support at power off\n",
			panel_type);

	return 0;
}

static int mipi_dsi_3d_barrier_sysfs_register(struct device *dev);
static int barrier_mode;

static int mipi_novatek_lcd_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		mipi_novatek_pdata = pdev->dev.platform_data;

		if (mipi_novatek_pdata && mipi_novatek_pdata->fpga_3d_config_addr)
			mipi_novatek_3d_init(mipi_novatek_pdata->fpga_3d_config_addr);

		/* create sysfs to control 3D barrier for the Sharp panel */
		if (mipi_dsi_3d_barrier_sysfs_register(&pdev->dev)) {
			pr_err("%s: Failed to register 3d Barrier sysfs\n",
					__func__);
			return -ENODEV;
		}
		barrier_mode = 0;

		return 0;
	}

	msm_fb_add_device(pdev);

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
	.set_backlight  = mipi_novatek_set_backlight,
	.display_on  = mipi_novatek_display_on,
	.bklswitch	= mipi_novatek_bkl_switch,
	.bklctrl	= mipi_novatek_bkl_ctrl,
	.panel_type_detect = mipi_novatek_panel_type_detect,
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
	__ATTR(enable_3d_barrier, 0644, mipi_dsi_3d_barrier_read,
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

static int __init mipi_novatek_lcd_init(void)
{
	mipi_dsi_buf_alloc(&novatek_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&novatek_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_novatek_lcd_init);
