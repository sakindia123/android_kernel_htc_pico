/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
#include <mach/panel_id.h>
#include <mach/htc_battery_common.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_orise.h"
#include <mach/debug_display.h>

// -----------------------------------------------------------------------------
//                         External routine declaration
// -----------------------------------------------------------------------------
extern int mipi_status;
#define DEFAULT_BRIGHTNESS 143
extern int bl_level_prevset;
extern struct mutex cmdlock;
extern struct dsi_cmd_desc *mipi_power_on_cmd;
extern struct dsi_cmd_desc *mipi_power_off_cmd;
extern int mipi_power_on_cmd_size;
extern int mipi_power_off_cmd_size;
extern char ptype[60];

static struct msm_panel_common_pdata *mipi_orise_pdata;

static struct dsi_buf orise_tx_buf;
static struct dsi_buf orise_rx_buf;

static int golfu_pwm_freq_22k = 1;

// -----------------------------------------------------------------------------
//                             Constant value define
// -----------------------------------------------------------------------------
static unsigned char led_pwm1[] = { 0x51, 0xf0 }; /* DTYPE_DCS_WRITE1 */ //PWM

static unsigned char bkl_enable_cmds[] = { 0x53, 0x24 };/* DTYPE_DCS_WRITE1 */ //bkl on and no dim
static unsigned char bkl_disable_cmds[] = { 0x53, 0x00 };/* DTYPE_DCS_WRITE1 */ //bkl off

static char all_pixel_off[] = { 0x22, 0x00 }; /* DTYPE_DCS_WRITE */
static char normal_display_mode_on[] = { 0x13, 0x00 }; /* DTYPE_DCS_WRITE */

static char exit_sleep[] = { 0x11, 0x00 }; /* DTYPE_DCS_WRITE */
static char display_on[] = { 0x29, 0x00 }; /* DTYPE_DCS_WRITE */

static char enable_te[] = { 0x35, 0x00 }; /* DTYPE_DCS_WRITE1 */
static char enable_cabc[] = { 0x55, 0x01 }; /* DTYPE_DCS_WRITE1 */

static char display_off[] = { 0x28, 0x00 }; /* DTYPE_DCS_WRITE */

/*golfu_auo_orise*/
static unsigned char golfu_auo_orise_001[] = { 0xFF, 0x48, 0x02, 0x01, }; /* DTYPE_DCS_LWRITE */
static unsigned char golfu_auo_orise_002[] = { 0x00, 0x80 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_003[] = { 0xFF, 0x48, 0x02, 0xff }; /* DTYPE_DCS_LWRITE */
/*-----------For C2 parameter start----------*/
static unsigned char golfu_auo_orise_c2_001[] = { 0x00, 0xb7 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_c2_002[] = { 0xb0, 0x14 }; /* DTYPE_DCS_WRITE1 */
/*-----------For C2 parameter end -----------*/
static unsigned char golfu_auo_orise_004[] = { 0xFF, 0x48, 0x02, 0x01 }; /* DTYPE_DCS_WRITE1 */
//Display on will be triggered later
//static unsigned char golfu_auo_orise_005[] = { 0x00, 0x00 }; /* DTYPE_DCS_WRITE1 */
//static unsigned char golfu_auo_orise_006[] ={0x51, 0xf0}; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_007[] = { 0x00, 0x00 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_008[] = { 0x53, 0x24 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_009[] = { 0x00, 0xB4 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_010[] = { 0xc6, 0x0a }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_c2_010[] = { 0xc6, 0x12 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_c3_010[] = { 0xc6, 0x12 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_011[] = { 0x00, 0xb1 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_012[] = { 0xc6, 0x05 }; /* DTYPE_DCS_WRITE1 PWM Freq=22 kHz*/
static unsigned char golfu_auo_orise_012_8k[] = { 0xc6, 0x10 }; /* DTYPE_DCS_WRITE1 PWM Freq= 8kHz*/
static unsigned char golfu_auo_orise_013[] = { 0x00, 0xb1 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_014[] = { 0xc5, 0x10 }; /* DTYPE_DCS_WRITE1 */
/*----------flicker-----------------*/
static unsigned char golfu_auo_orise_f01[] = { 0x00, 0x93 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_f02[] = { 0xc5, 0x37 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_f03[] = { 0x00, 0x81 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_f04[] = { 0xc4, 0x00 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_f05[] = { 0x00, 0x89 }; /* DTYPE_DCS_WRITE1 VCOM pull GND in sleep in */
static unsigned char golfu_auo_orise_f06[] = { 0xf5, 0x10 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_f07[] = { 0x00, 0x80 }; /* DTYPE_DCS_WRITE1 Source pull GND in non-display */
static unsigned char golfu_auo_orise_f08[] = { 0xc4, 0x20 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_f09[] = { 0x00, 0xa0 }; /* DTYPE_DCS_WRITE1 3 frame blank */
static unsigned char golfu_auo_orise_f10[] = { 0xc0, 0x03 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_f11[] = { 0x00, 0xa6 }; /* DTYPE_DCS_WRITE1 Zigzag Off write black pattern */
static unsigned char golfu_auo_orise_f12[] = { 0xb3, 0x20 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_f13[] = { 0x00, 0xa7 }; /* DTYPE_DCS_WRITE1 Zigzag Off write black pattern */
static unsigned char golfu_auo_orise_f14[] = { 0xb3, 0x01 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_f15[] = { 0x00, 0xb4 }; /* DTYPE_DCS_WRITE1 1+2 dot Inversion write black pattern */
static unsigned char golfu_auo_orise_f16[] = { 0xc0, 0x15 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_f17[] = { 0x00, 0x82 }; /* DTYPE_DCS_WRITE1 Disable LVD */
static unsigned char golfu_auo_orise_f18[] = { 0xf5, 0x00 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_f19[] = { 0x00, 0x83 }; /* DTYPE_DCS_WRITE1 Disable LVD */
static unsigned char golfu_auo_orise_f20[] = { 0xf5, 0x00 }; /* DTYPE_DCS_WRITE1 */
/*----------------------------------*/
static unsigned char golfu_auo_orise_015[] = { 0x00, 0xa0 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_016[] = { 0xb0, 0x42 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_017[] = { 0x00, 0xb0 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_018[] = { 0xc4, 0x02, 0x08, 0x05, 0x00, 0xff,0xff, 0xff }; /* DTYPE_DCS_LWRITE */
static unsigned char golfu_auo_orise_019[] = { 0x00, 0x90 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_020[] = { 0xc0, 0x00, 0x0f, 0x00, 0x15, 0x00,0x17, 0xff }; /* DTYPE_DCS_LWRITE */
static unsigned char golfu_auo_orise_021[] = { 0x00, 0x82 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_022[] = { 0xc5, 0x01 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_023[] = { 0x00, 0x90 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_024[] = { 0xc5, 0x47 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_025[] = { 0x00, 0x00 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_026[] = { 0xd8, 0x58, 0x58, 0xff }; /* DTYPE_DCS_LWRITE */
static unsigned char golfu_auo_orise_027[] = { 0x00, 0x91 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_028[] = { 0xb3, 0xc0, 0x25, 0xff }; /* DTYPE_DCS_LWRITE */
//static unsigned char golfu_auo_orise_028_1[] = { 0x00, 0x00 }; /* DTYPE_DCS_WRITE1 */
//static unsigned char golfu_auo_orise_028_2[] = { 0x36, 0x00 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_029[] = { 0x00, 0x00 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_030[] = { 0x00, 0x00 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_R[] = { 0x00, 0x00 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_G[] = { 0x00, 0x00 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_B[] = { 0x00, 0x00 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_031[] = { 0x00, 0x00 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_032[] = { 0x00, 0x00 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_033[] = { 0x00, 0x80 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_034[] = { 0xFF, 0x00, 0x00, 0xff }; /* DTYPE_DCS_LWRITE */
static unsigned char golfu_auo_orise_035[] = { 0x00, 0x00 }; /* DTYPE_DCS_WRITE1 */
static unsigned char golfu_auo_orise_036[] = { 0xFF, 0xFF, 0xFF, 0xFF }; /* DTYPE_DCS_LWRITE */

/* golfu_AUO_GAMMA */
static unsigned char golfu_auo_gamma22_01_positive[] = { 0xe1, 0x00, 0x05, 0x09,
		0x04, 0x02, 0x0b, 0x0a, 0x09, 0x05, 0x08, 0x10, 0x05, 0x06, 0x11, 0x09,
		0x01, 0xff, 0xff, 0xff, }; /* DTYPE_DCS_LWRITE */

static unsigned char golfu_auo_gamma22_01_negative[] = { 0xe2, 0x00, 0x05, 0x09,
		0x04, 0x02, 0x0b, 0x0a, 0x09, 0x05, 0x08, 0x10, 0x05, 0x06, 0x11, 0x09,
		0x01, 0xff, 0xff, 0xff, }; /* DTYPE_DCS_LWRITE */
/* golfu_AUO_GAMMA */

/* golfu_C2_AUO_GAMMA */
static unsigned char golfu_auo_c2_gamma22_01_positive[] = { 0xe1, 0x00, 0x05, 0x0b,
		0x04, 0x02, 0x0b, 0x0a, 0x09, 0x05, 0x08, 0x10, 0x05, 0x06, 0x0f, 0x09,
		0x01, 0xff, 0xff, 0xff, }; /* DTYPE_DCS_LWRITE */

static unsigned char golfu_auo_c2_gamma22_01_negative[] = { 0xe2, 0x00, 0x05, 0x0b,
		0x04, 0x02, 0x0b, 0x0a, 0x09, 0x05, 0x08, 0x10, 0x05, 0x06, 0x0f, 0x09,
		0x01, 0xff, 0xff, 0xff, }; /* DTYPE_DCS_LWRITE */
/* golfu_C2_AUO_GAMMA */

/* golfu_C3_AUO_GAMMA */
static unsigned char golfu_auo_c3_gamma22_01_positive[] = { 0xe1, 0x00, 0x08, 0x0e,
		0x02, 0x01, 0x0b, 0x0a, 0x09, 0x05, 0x08, 0x10, 0x05, 0x06, 0x0c, 0x09,
		0x01, 0xff, 0xff, 0xff, }; /* DTYPE_DCS_LWRITE */

static unsigned char golfu_auo_c3_gamma22_01_negative[] = { 0xe2, 0x00, 0x08, 0x0e,
		0x02, 0x01, 0x0b, 0x0a, 0x09, 0x05, 0x08, 0x10, 0x05, 0x06, 0x0c, 0x09,
		0x01, 0xff, 0xff, 0xff, }; /* DTYPE_DCS_LWRITE */
/* golfu_C3_AUO_GAMMA */

/* golfu_C4_AUO_GAMMA */
static unsigned char golfu_auo_c4_gamma22_01_positive[] = { 0xe1, 0x00, 0x08, 0x0e,
        0x02, 0x01, 0x0b, 0x0a, 0x09, 0x05, 0x08, 0x10, 0x05, 0x06, 0x0b, 0x09,
        0x01, 0xff, 0xff, 0xff, }; /* DTYPE_DCS_LWRITE */

static unsigned char golfu_auo_c4_gamma22_01_negative[] = { 0xe2, 0x00, 0x08, 0x0e,
		0x02, 0x01, 0x0b, 0x0a, 0x09, 0x05, 0x08, 0x10, 0x05, 0x06, 0x0b, 0x09,
		0x01, 0xff, 0xff, 0xff, }; /* DTYPE_DCS_LWRITE */
/* golfu_C4_AUO_GAMMA */


/*GOLFU C2 RGB digital gamma setting*/
static unsigned char golfu_auo_c2_R_gamma[] = {0xec, 0x40, 0x44, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0xff, 0xff, 0xff}; /* DTYPE_DCS_LWRITE */
static unsigned char golfu_auo_c2_G_gamma[] = {0xed, 0x00, 0x65, 0x44, 0x34, 0x44, 0x44,
		0x44, 0x43, 0x44, 0x44, 0x34, 0x44, 0x34, 0x44, 0x44, 0x34, 0x44, 0x34, 0x54,
		0x34, 0x44, 0x43, 0x54, 0x33, 0x44, 0x43, 0x44, 0x44, 0x44, 0x33, 0x44, 0x36,
		0xff, 0xff, 0xff}; /* DTYPE_DCS_LWRITE */
static unsigned char golfu_auo_c2_B_gamma[] = {0xee, 0x00, 0x47, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0xff, 0xff, 0xff}; /* DTYPE_DCS_LWRITE */
/*GOLFU C2 RGB digital gamma setting*/

/*GOLFU C3 RGB digital gamma setting*/
static unsigned char golfu_auo_c3_R_gamma[] = {0xec, 0x40, 0x44, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0xff, 0xff, 0xff}; /* DTYPE_DCS_LWRITE */
static unsigned char golfu_auo_c3_G_gamma[] = {0xed, 0x00, 0x81, 0x54, 0x34, 0x44, 0x44,
		0x44, 0x43, 0x44, 0x44, 0x34, 0x44, 0x44, 0x44, 0x34, 0x44, 0x44, 0x34, 0x44,
		0x44, 0x44, 0x43, 0x44, 0x44, 0x43, 0x43, 0x54, 0x34, 0x44, 0x44, 0x32, 0x76,
		0xff, 0xff, 0xff}; /* DTYPE_DCS_LWRITE */
static unsigned char golfu_auo_c3_B_gamma[] = {0xee, 0x01, 0x44, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0xff, 0xff, 0xff}; /* DTYPE_DCS_LWRITE */
/*GOLFU C3 RGB digital gamma setting*/

/*GOLFU C4 RGB digital gamma Wx decay 0.025 and Wy decay 0.025 based on Rev 1.1*/
static unsigned char golfu_auo_c4_R_gamma[] = {0xec, 0x00, 0x00, 0x00, 0x6b, 0x44, 0x54,
        0x43, 0x33, 0x43, 0x44, 0x44, 0x33, 0x33, 0x34, 0x44, 0x43, 0x33, 0x43, 0x43,
        0x43, 0x43, 0x33, 0x34, 0x33, 0x34, 0x33, 0x43, 0x43, 0x43, 0x34, 0x23, 0x45,
        0xff, 0xff, 0xff}; /* DTYPE_DCS_LWRITE */
static unsigned char golfu_auo_c4_G_gamma[] = {0xed, 0x00, 0x70, 0x54, 0x34, 0x44, 0x44,
		0x43, 0x43, 0x34, 0x44, 0x44, 0x33, 0x44, 0x43, 0x44, 0x43, 0x43, 0x43, 0x43,
		0x44, 0x34, 0x43, 0x34, 0x44, 0x33, 0x34, 0x53, 0x44, 0x44, 0x43, 0x32, 0x54,
		0xff, 0xff, 0xff}; /* DTYPE_DCS_LWRITE */
static unsigned char golfu_auo_c4_B_gamma[] = {0xee, 0x30, 0x44, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0xff, 0xff, 0xff}; /* DTYPE_DCS_LWRITE */
/*GOLFU C4 RGB digital gamma Wx decay 0.025 and Wy decay 0.025 setting based on Rev 1.1*/

/*primods_dd_sharp_orise*/
static char pro_001[] = {0xff, 0x80, 0x09, 0x01, 0x01};
static char pro_002[] = {0x00, 0x80};
static char pro_003[] = {0xff, 0x80, 0x09};
static char pro_004[] = {0x00, 0x97};
static char pro_005[] = {0xce, 0x26};
static char pro_006[] = {0x00, 0x9a};
static char pro_007[] = {0xce, 0x27};
static char pro_008[] = {0x00, 0xc7};
static char pro_009[] = {0xcf, 0x88};
static char pro_010[] = {0x00, 0xb1};
static char pro_011[] = {0xc6, 0x0a};
static char pro_012[] = {0x00, 0x00};
static char pro_gamma_positive[] = {
	0xE1, 0x00, 0x0a, 0x11,
	0x0f, 0x09, 0x11, 0x0d,
	0x0b, 0x02, 0x06, 0x06,
	0x03, 0x10, 0x14, 0x10,
	0x06};
static char pro_gamma_negative[] = {
	0xE2, 0x00, 0x0a, 0x11,
	0x0f, 0x09, 0x11, 0x0d,
	0x0b, 0x02, 0x06, 0x06,
	0x03, 0x10, 0x14, 0x10,
	0x06};
static char pro_gamma_red[] = {
	0xEC, 0x30, 0x53, 0x53,
	0x55, 0x34, 0x33, 0x43,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x34, 0x44,
	0x34, 0x44, 0x43, 0x54,
	0x44, 0x33, 0x43, 0x33,
	0x44, 0x54, 0x55, 0x34,
	0x44, 0x44};
static char pro_gamma_green[] = {
	0xED, 0x40, 0x44, 0x54,
	0x55, 0x44, 0x33, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x43, 0x44, 0x44, 0x44,
	0x44, 0x34, 0x44, 0x44,
	0x34, 0x55, 0x44, 0x44,
	0x44, 0x44};
static char pro_gamma_blue[] = {
	0xEE, 0x30, 0x53, 0x55,
	0x54, 0x44, 0x33, 0x43,
	0x44, 0x54, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x43, 0x43, 0x44, 0x45,
	0x43, 0x44, 0x44, 0x44,
	0x43, 0x55, 0x44, 0x44,
	0x34, 0x44};

static struct dsi_cmd_desc golfu_auo_orise_cmd_on_cmds[] = {
//	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(sw_reset), sw_reset},
	{ DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(exit_sleep), exit_sleep },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_001), golfu_auo_orise_001 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_002), golfu_auo_orise_002 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_003), golfu_auo_orise_003 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_004), golfu_auo_orise_004 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_007), golfu_auo_orise_007 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_008), golfu_auo_orise_008 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_009), golfu_auo_orise_009 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_010), golfu_auo_orise_010 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_011), golfu_auo_orise_011 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_012), golfu_auo_orise_012 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_013), golfu_auo_orise_013 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_014), golfu_auo_orise_014 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f01), golfu_auo_orise_f01 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f02), golfu_auo_orise_f02 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f03), golfu_auo_orise_f03 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f04), golfu_auo_orise_f04 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f05), golfu_auo_orise_f05 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f06), golfu_auo_orise_f06 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f07), golfu_auo_orise_f07 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f08), golfu_auo_orise_f08 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f09), golfu_auo_orise_f09 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f10), golfu_auo_orise_f10 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_015), golfu_auo_orise_015 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_016), golfu_auo_orise_016 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_017), golfu_auo_orise_017 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_018), golfu_auo_orise_018 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_019), golfu_auo_orise_019 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_020), golfu_auo_orise_020 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_021), golfu_auo_orise_021 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_022), golfu_auo_orise_022 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_023), golfu_auo_orise_023 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_024), golfu_auo_orise_024 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_025), golfu_auo_orise_025 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_026), golfu_auo_orise_026 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_027), golfu_auo_orise_027 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_028), golfu_auo_orise_028 },
//	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(golfu_auo_orise_028_1), golfu_auo_orise_028_1},
//	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(golfu_auo_orise_028_2), golfu_auo_orise_028_2},
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_029), golfu_auo_orise_029 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_gamma22_01_positive), golfu_auo_gamma22_01_positive },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_030), golfu_auo_orise_030 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_gamma22_01_negative), golfu_auo_gamma22_01_negative },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_031), golfu_auo_orise_031 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_032), golfu_auo_orise_032 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_033), golfu_auo_orise_033 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_034), golfu_auo_orise_034 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_035), golfu_auo_orise_035 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_036), golfu_auo_orise_036 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(enable_te), enable_te },
	{ DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep },
	{ DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on },
};

static struct dsi_cmd_desc golfu_auo_orise_c2_cmd_on_cmds[] = {
//	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(sw_reset), sw_reset},
	{ DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(exit_sleep), exit_sleep },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_001), golfu_auo_orise_001 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_002), golfu_auo_orise_002 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_003), golfu_auo_orise_003 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_c2_001), golfu_auo_orise_c2_001 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_c2_002), golfu_auo_orise_c2_002 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_007), golfu_auo_orise_007 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_008), golfu_auo_orise_008 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_009), golfu_auo_orise_009 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_c2_010), golfu_auo_orise_c2_010 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_011), golfu_auo_orise_011 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_012), golfu_auo_orise_012 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_013), golfu_auo_orise_013 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_014), golfu_auo_orise_014 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f01), golfu_auo_orise_f01 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f02), golfu_auo_orise_f02 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f03), golfu_auo_orise_f03 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f04), golfu_auo_orise_f04 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f05), golfu_auo_orise_f05 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f06), golfu_auo_orise_f06 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f07), golfu_auo_orise_f07 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f08), golfu_auo_orise_f08 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f09), golfu_auo_orise_f09 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f10), golfu_auo_orise_f10 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_015), golfu_auo_orise_015 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_016), golfu_auo_orise_016 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_017), golfu_auo_orise_017 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_018), golfu_auo_orise_018 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_019), golfu_auo_orise_019 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_020), golfu_auo_orise_020 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_021), golfu_auo_orise_021 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_022), golfu_auo_orise_022 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_023), golfu_auo_orise_023 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_024), golfu_auo_orise_024 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_025), golfu_auo_orise_025 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_026), golfu_auo_orise_026 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_027), golfu_auo_orise_027 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_028), golfu_auo_orise_028 },
//	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(golfu_auo_orise_028_1), golfu_auo_orise_028_1},
//	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(golfu_auo_orise_028_2), golfu_auo_orise_028_2},
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_029), golfu_auo_orise_029 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_c2_gamma22_01_positive), golfu_auo_c2_gamma22_01_positive },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_030), golfu_auo_orise_030 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_c2_gamma22_01_negative), golfu_auo_c2_gamma22_01_negative },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_R), golfu_auo_orise_R },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_c2_R_gamma), golfu_auo_c2_R_gamma },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_G), golfu_auo_orise_G },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_c2_G_gamma), golfu_auo_c2_G_gamma },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_B), golfu_auo_orise_B },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_c2_B_gamma), golfu_auo_c2_B_gamma },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_031), golfu_auo_orise_031 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_032), golfu_auo_orise_032 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_033), golfu_auo_orise_033 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_034), golfu_auo_orise_034 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_035), golfu_auo_orise_035 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_036), golfu_auo_orise_036 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(enable_te), enable_te },
	{ DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep },
	{ DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on },
};

static struct dsi_cmd_desc golfu_auo_orise_c3_cmd_on_cmds[] = {
//	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(sw_reset), sw_reset},
	{ DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(exit_sleep), exit_sleep },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_001), golfu_auo_orise_001 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_002), golfu_auo_orise_002 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_003), golfu_auo_orise_003 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_c2_001), golfu_auo_orise_c2_001 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_c2_002), golfu_auo_orise_c2_002 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_007), golfu_auo_orise_007 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_008), golfu_auo_orise_008 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_009), golfu_auo_orise_009 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_c3_010), golfu_auo_orise_c3_010 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_011), golfu_auo_orise_011 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_012), golfu_auo_orise_012 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_013), golfu_auo_orise_013 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_014), golfu_auo_orise_014 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f01), golfu_auo_orise_f01 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f02), golfu_auo_orise_f02 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f03), golfu_auo_orise_f03 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f04), golfu_auo_orise_f04 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f05), golfu_auo_orise_f05 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f06), golfu_auo_orise_f06 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f07), golfu_auo_orise_f07 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f08), golfu_auo_orise_f08 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f09), golfu_auo_orise_f09 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f10), golfu_auo_orise_f10 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_015), golfu_auo_orise_015 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_016), golfu_auo_orise_016 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_017), golfu_auo_orise_017 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_018), golfu_auo_orise_018 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_019), golfu_auo_orise_019 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_020), golfu_auo_orise_020 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_021), golfu_auo_orise_021 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_022), golfu_auo_orise_022 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_023), golfu_auo_orise_023 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_024), golfu_auo_orise_024 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_025), golfu_auo_orise_025 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_026), golfu_auo_orise_026 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_027), golfu_auo_orise_027 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_028), golfu_auo_orise_028 },
//	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(golfu_auo_orise_028_1), golfu_auo_orise_028_1},
//	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(golfu_auo_orise_028_2), golfu_auo_orise_028_2},
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_029), golfu_auo_orise_029 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_c3_gamma22_01_positive), golfu_auo_c3_gamma22_01_positive },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_030), golfu_auo_orise_030 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_c3_gamma22_01_negative), golfu_auo_c3_gamma22_01_negative },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_R), golfu_auo_orise_R },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_c3_R_gamma), golfu_auo_c3_R_gamma },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_G), golfu_auo_orise_G },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_c3_G_gamma), golfu_auo_c3_G_gamma },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_B), golfu_auo_orise_B },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_c3_B_gamma), golfu_auo_c3_B_gamma },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_031), golfu_auo_orise_031 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_032), golfu_auo_orise_032 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_033), golfu_auo_orise_033 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_034), golfu_auo_orise_034 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_035), golfu_auo_orise_035 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_036), golfu_auo_orise_036 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(enable_te), enable_te },
	{ DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep },
	{ DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on },
};

static struct dsi_cmd_desc golfu_auo_orise_c4_cmd_on_cmds[] = {
//  {DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(sw_reset), sw_reset},
	{ DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(exit_sleep), exit_sleep },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_001), golfu_auo_orise_001 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_002), golfu_auo_orise_002 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_003), golfu_auo_orise_003 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_c2_001), golfu_auo_orise_c2_001 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_c2_002), golfu_auo_orise_c2_002 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_007), golfu_auo_orise_007 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_008), golfu_auo_orise_008 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_009), golfu_auo_orise_009 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_c3_010), golfu_auo_orise_c3_010 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_011), golfu_auo_orise_011 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_012), golfu_auo_orise_012 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_013), golfu_auo_orise_013 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_014), golfu_auo_orise_014 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f01), golfu_auo_orise_f01 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f02), golfu_auo_orise_f02 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f03), golfu_auo_orise_f03 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f04), golfu_auo_orise_f04 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f05), golfu_auo_orise_f05 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f06), golfu_auo_orise_f06 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f07), golfu_auo_orise_f07 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f08), golfu_auo_orise_f08 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f09), golfu_auo_orise_f09 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f10), golfu_auo_orise_f10 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_015), golfu_auo_orise_015 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_016), golfu_auo_orise_016 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_017), golfu_auo_orise_017 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_018), golfu_auo_orise_018 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_019), golfu_auo_orise_019 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_020), golfu_auo_orise_020 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_021), golfu_auo_orise_021 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_022), golfu_auo_orise_022 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_023), golfu_auo_orise_023 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_024), golfu_auo_orise_024 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_025), golfu_auo_orise_025 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_026), golfu_auo_orise_026 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_027), golfu_auo_orise_027 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_028), golfu_auo_orise_028 },
//  { DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_028_1), golfu_auo_orise_028_1},
//  { DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_028_2), golfu_auo_orise_028_2},
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_029), golfu_auo_orise_029 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_c4_gamma22_01_positive), golfu_auo_c4_gamma22_01_positive },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_030), golfu_auo_orise_030 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_c4_gamma22_01_negative), golfu_auo_c4_gamma22_01_negative },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_R), golfu_auo_orise_R },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_c4_R_gamma), golfu_auo_c4_R_gamma },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_G), golfu_auo_orise_G },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_c4_G_gamma), golfu_auo_c4_G_gamma },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_B), golfu_auo_orise_B },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_c4_B_gamma), golfu_auo_c4_B_gamma },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_031), golfu_auo_orise_031 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_032), golfu_auo_orise_032 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_033), golfu_auo_orise_033 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_034), golfu_auo_orise_034 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_035), golfu_auo_orise_035 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_036), golfu_auo_orise_036 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(enable_te), enable_te },
	{ DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep },
	{ DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on },
};

static struct dsi_cmd_desc primods_sharp_orise_video_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(pro_001), pro_001},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_002), pro_002},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(pro_003), pro_003},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_004), pro_004},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_005), pro_005},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_006), pro_006},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_007), pro_007},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_008), pro_008},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_009), pro_009},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_010), pro_010},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_011), pro_011},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_012), pro_012},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(pro_gamma_positive), pro_gamma_positive},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(pro_gamma_negative), pro_gamma_negative},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(pro_gamma_red), pro_gamma_red},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(pro_gamma_green), pro_gamma_green},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(pro_gamma_blue), pro_gamma_blue},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(bkl_enable_cmds), bkl_enable_cmds},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(enable_cabc), enable_cabc},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc primods_sharp_C1_orise_video_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(bkl_enable_cmds), bkl_enable_cmds},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(enable_cabc), enable_cabc},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc orise_display_off_cmds[] = {
	{ DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(all_pixel_off), all_pixel_off },
	/* Enter ORISE Mode*/
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_001), golfu_auo_orise_001 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_002), golfu_auo_orise_002 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_003), golfu_auo_orise_003 },
	/* Zigzag off & 1+2 dot Inversion */
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f11), golfu_auo_orise_f11 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f12), golfu_auo_orise_f12 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f13), golfu_auo_orise_f13 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f14), golfu_auo_orise_f14 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f15), golfu_auo_orise_f15 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 120, sizeof(golfu_auo_orise_f16), golfu_auo_orise_f16 },
	/* Source = 0V*/
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f07), golfu_auo_orise_f07 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f08), golfu_auo_orise_f08 },
	/* Display off */
	{ DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_off), display_off },
	/* Normal Display Mode On */
	{ DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(normal_display_mode_on), normal_display_mode_on },
	/* Disable LVD */
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f17), golfu_auo_orise_f17 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f18), golfu_auo_orise_f18 },
	/* Disable LVD */
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_f19), golfu_auo_orise_f19 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 100, sizeof(golfu_auo_orise_f20), golfu_auo_orise_f20 },
	/* Leave ORISE Mode  */
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_033), golfu_auo_orise_033 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_034), golfu_auo_orise_034 },
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_035), golfu_auo_orise_035 },
	{ DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_036), golfu_auo_orise_036 },
};

static struct dsi_cmd_desc orise_pwm_freq_8k_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_001), golfu_auo_orise_001},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_002), golfu_auo_orise_002},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_003), golfu_auo_orise_003},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_009), golfu_auo_orise_009},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_c2_010), golfu_auo_orise_c2_010},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_011), golfu_auo_orise_011},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_012_8k), golfu_auo_orise_012_8k}, /* PWM Freq=8 kHz*/
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_033), golfu_auo_orise_033},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_034), golfu_auo_orise_034},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_035), golfu_auo_orise_035},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_036), golfu_auo_orise_036},
};

static struct dsi_cmd_desc orise_pwm_freq_22k_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_001), golfu_auo_orise_001},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_002), golfu_auo_orise_002},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_003), golfu_auo_orise_003},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_009), golfu_auo_orise_009},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_c2_010), golfu_auo_orise_c2_010},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_011), golfu_auo_orise_011},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_012), golfu_auo_orise_012}, /* PWM Freq=22 kHz*/
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_033), golfu_auo_orise_033},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_034), golfu_auo_orise_034},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(golfu_auo_orise_035), golfu_auo_orise_035},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(golfu_auo_orise_036), golfu_auo_orise_036},
};

//static char manufacture_id[2] = {0x04, 0x00}; /* DTYPE_DCS_READ */
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

static void mipi_orise_3d_init(int addr) {
	fpga_addr = addr;
}

static void mipi_dsi_enable_3d_barrier(int mode) {
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

static struct dsi_cmd_desc orise_cmd_backlight_cmds[] = {
/* VIDEO mode need use LWRITE to send cmd */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(led_pwm1), led_pwm1},
};

static struct dsi_cmd_desc orise_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc orise_bkl_enable_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(bkl_enable_cmds), bkl_enable_cmds},
};

static struct dsi_cmd_desc orise_bkl_disable_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(bkl_disable_cmds), bkl_disable_cmds},
};

void mipi_orise_panel_type_detect(struct mipi_panel_info *mipi) {
	if (panel_type == PANEL_ID_GOLFU_AUO) {
		PR_DISP_INFO("%s: panel_type=PANEL_ID_GOLFU_AUO\n", __func__);
		strcat(ptype, "PANEL_ID_GOLFU_AUO");
		mipi_power_on_cmd = golfu_auo_orise_cmd_on_cmds;
		mipi_power_on_cmd_size = ARRAY_SIZE(golfu_auo_orise_cmd_on_cmds);
		mipi_power_off_cmd = orise_display_off_cmds;
		mipi_power_off_cmd_size = ARRAY_SIZE(orise_display_off_cmds);
	} else if (panel_type == PANEL_ID_GOLFU_AUO_C2) {
		PR_DISP_INFO("%s: panel_type=PANEL_ID_GOLFU_AUO_C2\n", __func__);
		strcat(ptype, "PANEL_ID_GOLFU_AUO_C2");
		mipi_power_on_cmd = golfu_auo_orise_c2_cmd_on_cmds;
		mipi_power_on_cmd_size = ARRAY_SIZE(golfu_auo_orise_c2_cmd_on_cmds);
		mipi_power_off_cmd = orise_display_off_cmds;
		mipi_power_off_cmd_size = ARRAY_SIZE(orise_display_off_cmds);
	} else if (panel_type == PANEL_ID_GOLFU_AUO_C3) {
		PR_DISP_INFO("%s: panel_type=PANEL_ID_GOLFU_AUO_C3\n", __func__);
		strcat(ptype, "PANEL_ID_GOLFU_AUO_C3");
		mipi_power_on_cmd = golfu_auo_orise_c3_cmd_on_cmds;
		mipi_power_on_cmd_size = ARRAY_SIZE(golfu_auo_orise_c3_cmd_on_cmds);
		mipi_power_off_cmd = orise_display_off_cmds;
		mipi_power_off_cmd_size = ARRAY_SIZE(orise_display_off_cmds);
	} else if (panel_type == PANEL_ID_GOLFU_AUO_C4) {
		PR_DISP_INFO("%s: panel_type=PANEL_ID_GOLFU_AUO_C4\n", __func__);
		strcat(ptype, "PANEL_ID_GOLFU_AUO_C4");
		mipi_power_on_cmd = golfu_auo_orise_c4_cmd_on_cmds;
		mipi_power_on_cmd_size = ARRAY_SIZE(golfu_auo_orise_c4_cmd_on_cmds);
		mipi_power_off_cmd = orise_display_off_cmds;
		mipi_power_off_cmd_size = ARRAY_SIZE(orise_display_off_cmds);
	} else if (panel_type == PANEL_ID_PRIMODS_SHARP) {
		PR_DISP_INFO("%s: panel_type=PANEL_ID_PRIMODS_SHARP\n", __func__);
		strcat(ptype, "PANEL_ID_PRIMODS_SHARP");
		mipi_power_on_cmd = primods_sharp_orise_video_on_cmds;
		mipi_power_on_cmd_size = ARRAY_SIZE(primods_sharp_orise_video_on_cmds);
		mipi_power_off_cmd = orise_display_off_cmds;
		mipi_power_off_cmd_size = ARRAY_SIZE(orise_display_off_cmds);
	} else if (panel_type == PANEL_ID_PRIMODS_SHARP_C1) {
		PR_DISP_INFO("%s: panel_type=PANEL_ID_PRIMODS_SHARP_C1\n", __func__);
		strcat(ptype, "PANEL_ID_PRIMODS_SHARP_C1");
		mipi_power_on_cmd = primods_sharp_C1_orise_video_on_cmds;
		mipi_power_on_cmd_size = ARRAY_SIZE(primods_sharp_C1_orise_video_on_cmds);
		mipi_power_off_cmd = orise_display_off_cmds;
		mipi_power_off_cmd_size = ARRAY_SIZE(orise_display_off_cmds);
	} else if (panel_type == PANEL_ID_PRIMODD_SHARP) {
		PR_DISP_INFO("%s: panel_type=PANEL_ID_PRIMODD_SHARP\n", __func__);
		strcat(ptype, "PANEL_ID_PRIMODD_SHARP");
		mipi_power_on_cmd = primods_sharp_orise_video_on_cmds;
		mipi_power_on_cmd_size = ARRAY_SIZE(primods_sharp_orise_video_on_cmds);
		mipi_power_off_cmd = orise_display_off_cmds;
		mipi_power_off_cmd_size = ARRAY_SIZE(orise_display_off_cmds);
	} else if (panel_type == PANEL_ID_PRIMODD_SHARP_C1) {
		PR_DISP_INFO("%s: panel_type=PANEL_ID_PRIMODD_SHARP_C1\n", __func__);
		strcat(ptype, "PANEL_ID_PRIMODD_SHARP_C1");
		mipi_power_on_cmd = primods_sharp_C1_orise_video_on_cmds;
		mipi_power_on_cmd_size = ARRAY_SIZE(primods_sharp_C1_orise_video_on_cmds);
		mipi_power_off_cmd = orise_display_off_cmds;
		mipi_power_off_cmd_size = ARRAY_SIZE(orise_display_off_cmds);
	} else {
		printk(KERN_ERR "%s: panel_type=0x%x not support\n", __func__, panel_type);
		strcat(ptype, "PANEL_ID_NONE");
	}
	return;
}

static int mipi_dsi_set_backlight(struct msm_fb_data_type *mfd) {
	struct mipi_panel_info *mipi;

	mipi = &mfd->panel_info.mipi;

	if (mipi_status == 0)
		goto end;

	if (mipi_orise_pdata && mipi_orise_pdata->shrink_pwm)
		led_pwm1[1] = mipi_orise_pdata->shrink_pwm(mfd->bl_level);
	else
		led_pwm1[1] = (unsigned char) (mfd->bl_level);

	if (mfd->bl_level == 0 || board_mfg_mode() == 4
			|| (board_mfg_mode() == 5 && !(htc_battery_get_zcharge_mode() % 2))) {
		led_pwm1[1] = 0;
	}

	htc_mdp_sem_down(current, &mfd->dma->mutex);
	if (mipi->mode == DSI_VIDEO_MODE) {
		mipi_dsi_cmds_tx(mfd, &orise_tx_buf, orise_cmd_backlight_cmds,
				ARRAY_SIZE(orise_cmd_backlight_cmds));
	} else {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);

		/* Dynamic change the pwm freq. to avoid affecting the audio */
		if (panel_type == PANEL_ID_GOLFU_AUO_C2 || panel_type == PANEL_ID_GOLFU_AUO_C3 || panel_type == PANEL_ID_GOLFU_AUO_C4) {
			if (mfd->bl_level <= 30 && golfu_pwm_freq_22k == 1) {
				mipi_dsi_cmds_tx(mfd, &orise_tx_buf, orise_pwm_freq_8k_cmds,
							ARRAY_SIZE(orise_pwm_freq_8k_cmds));
				golfu_pwm_freq_22k = 0;
				PR_DISP_DEBUG("[DISP] backlight setting PWM Freq 8k\n");
			} else if (mfd->bl_level > 30 && golfu_pwm_freq_22k == 0){
				mipi_dsi_cmds_tx(mfd, &orise_tx_buf, orise_pwm_freq_22k_cmds,
							ARRAY_SIZE(orise_pwm_freq_22k_cmds));
				golfu_pwm_freq_22k = 1;
				PR_DISP_DEBUG("[DISP] backlight setting PWM Freq 22k\n");
			}
		}

		mipi_dsi_cmds_tx(mfd, &orise_tx_buf, orise_cmd_backlight_cmds,
				ARRAY_SIZE(orise_cmd_backlight_cmds));
	}
	htc_mdp_sem_up(&mfd->dma->mutex);

	if (led_pwm1[1] != 0)
		bl_level_prevset = mfd->bl_level;

	PR_DISP_DEBUG("mipi_dsi_set_backlight > set brightness to %d\n", led_pwm1[1]);

	end: return 0;
}

static void mipi_orise_set_backlight(struct msm_fb_data_type *mfd) {
	if (!mfd->panel_power_on)
		return;

	mipi_dsi_set_backlight(mfd);
}

static void mipi_orise_display_on(struct msm_fb_data_type *mfd) {
	struct mipi_panel_info *mipi;
	mipi = &mfd->panel_info.mipi;

	PR_DISP_DEBUG("%s+\n", __func__);

	if (mipi->mode == DSI_CMD_MODE) {
		htc_mdp_sem_down(current, &mfd->dma->mutex);
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
		mipi_dsi_cmds_tx(mfd, &orise_tx_buf, orise_display_on_cmds,
				ARRAY_SIZE(orise_display_on_cmds));
		htc_mdp_sem_up(&mfd->dma->mutex);
	}
}

static void mipi_orise_bkl_switch(struct msm_fb_data_type *mfd, bool on) {
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

static void mipi_orise_bkl_ctrl(struct msm_fb_data_type *mfd, bool on) {
	struct mipi_panel_info *mipi;
	mipi = &mfd->panel_info.mipi;

	PR_DISP_DEBUG("mipi_orise_bkl_ctrl > on = %x\n",on);

	if (mipi->mode == DSI_CMD_MODE) {
		htc_mdp_sem_down(current, &mfd->dma->mutex);
		if (on) {
			mipi_dsi_op_mode_config(DSI_CMD_MODE);
			mipi_dsi_cmds_tx(mfd, &orise_tx_buf, orise_bkl_enable_cmds,
					ARRAY_SIZE(orise_bkl_enable_cmds));
		} else {
			mipi_dsi_op_mode_config(DSI_CMD_MODE);
			mipi_dsi_cmds_tx(mfd, &orise_tx_buf, orise_bkl_disable_cmds,
					ARRAY_SIZE(orise_bkl_disable_cmds));
		}
		htc_mdp_sem_up(&mfd->dma->mutex);
	}
}

static int mipi_orise_lcd_on(struct platform_device *pdev) {
	struct msm_fb_data_type *mfd;
	struct msm_fb_panel_data *pdata = NULL;
	struct msm_panel_info *pinfo;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	pdata = (struct msm_fb_panel_data *) mfd->pdev->dev.platform_data;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	pinfo = &mfd->panel_info;
	if (pinfo->is_3d_panel)
		support_3d = TRUE;

	if (mfd->init_mipi_lcd == 0) {
		PR_DISP_DEBUG("Display On - 1st time\n");

		if (pdata && pdata->panel_type_detect)
			pdata->panel_type_detect(&pinfo->mipi);

		if (panel_type == PANEL_ID_PRIMODS_SHARP || panel_type == PANEL_ID_PRIMODS_SHARP_C1 ||
			panel_type == PANEL_ID_PRIMODD_SHARP || panel_type == PANEL_ID_PRIMODD_SHARP_C1)
			mipi_dsi_cmds_tx(mfd, &orise_tx_buf, mipi_power_on_cmd,
					mipi_power_on_cmd_size);

		mfd->init_mipi_lcd = 1;

	} else {
		PR_DISP_DEBUG("Display On \n");
		if (panel_type != PANEL_ID_NONE) {
			PR_DISP_INFO("%s\n", ptype);
			htc_mdp_sem_down(current, &mfd->dma->mutex);
			mipi_dsi_cmds_tx(mfd, &orise_tx_buf, mipi_power_on_cmd,
					mipi_power_on_cmd_size);

			/* Initialize the flag for golfu pwm freq. check */
			golfu_pwm_freq_22k = 1;
			htc_mdp_sem_up(&mfd->dma->mutex);

#if 0 /* mipi read command verify */
			/* clean up ack_err_status */
			mipi_dsi_cmd_bta_sw_trigger();
			mipi_orise_manufacture_id(mfd);
#endif
		} else {
			printk(KERN_ERR "panel_type=0x%x not support at power on\n", panel_type);
			return -EINVAL;
		}
	} PR_DISP_DEBUG("Init done!\n");

	return 0;
}

static int mipi_orise_lcd_off(struct platform_device *pdev) {
	struct msm_fb_data_type *mfd;

	PR_DISP_INFO("%s\n", __func__);

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (panel_type != PANEL_ID_NONE) {
		PR_DISP_INFO("%s\n", ptype);
		mipi_dsi_cmds_tx(mfd, &orise_tx_buf, mipi_power_off_cmd,
				mipi_power_off_cmd_size);
	} else
	printk(KERN_ERR "panel_type=0x%x not support at power off\n",
			panel_type);

	return 0;
}

/* PrimoDS/DD sharp panel workaround to fix the abnormal wave form  */
int mipi_orise_lcd_pre_off(struct platform_device *pdev) {
	struct msm_fb_data_type *mfd;

	PR_DISP_INFO("%s\n", __func__);

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(mfd, &orise_tx_buf, orise_display_off_cmds,
			ARRAY_SIZE(orise_display_off_cmds));

	return 0;
}

static int mipi_dsi_3d_barrier_sysfs_register(struct device *dev);
static int barrier_mode;

static int mipi_orise_lcd_probe(struct platform_device *pdev) {
	if (pdev->id == 0) {
		mipi_orise_pdata = pdev->dev.platform_data;

		if (mipi_orise_pdata && mipi_orise_pdata->fpga_3d_config_addr)
			mipi_orise_3d_init(mipi_orise_pdata->fpga_3d_config_addr);

		/* create sysfs to control 3D barrier for the Sharp panel */
		if (mipi_dsi_3d_barrier_sysfs_register(&pdev->dev)) {
			pr_err("%s: Failed to register 3d Barrier sysfs\n", __func__);
			return -ENODEV;
		}
		barrier_mode = 0;

		return 0;
	}

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe = mipi_orise_lcd_probe,
	.driver = {
		.name = "mipi_orise",
	},
};

static struct msm_fb_panel_data orise_panel_data = {
	.on = mipi_orise_lcd_on,
	.off = mipi_orise_lcd_off,
	.set_backlight = mipi_orise_set_backlight,
	.display_on = mipi_orise_display_on,
	.bklswitch = mipi_orise_bkl_switch,
	.bklctrl = mipi_orise_bkl_ctrl,
	.panel_type_detect = mipi_orise_panel_type_detect,
};

static ssize_t mipi_dsi_3d_barrier_read(struct device *dev,
		struct device_attribute *attr, char *buf) {
	return snprintf((char *) buf, sizeof(buf), "%u\n", barrier_mode);
}

static ssize_t mipi_dsi_3d_barrier_write(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count) {
	int ret = -1;
	u32 data = 0;

	if (sscanf((char *) buf, "%u", &data) != 1) {
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
				mipi_dsi_3d_barrier_write), };

static int mipi_dsi_3d_barrier_sysfs_register(struct device *dev) {
	int i;

	for (i = 0; i < ARRAY_SIZE(mipi_dsi_3d_barrier_attributes); i++)
		if (device_create_file(dev, mipi_dsi_3d_barrier_attributes + i))
			goto error;

	return 0;

	error: for (; i >= 0; i--)
		device_remove_file(dev, mipi_dsi_3d_barrier_attributes + i);
	pr_err("%s: Unable to create interface\n", __func__);

	return -ENODEV;
}

static int ch_used[3];

int mipi_orise_device_register(struct msm_panel_info *pinfo, u32 channel,
		u32 panel) {
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_orise", (panel << 8) | channel);
	if (!pdev)
		return -ENOMEM;

	orise_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &orise_panel_data,
			sizeof(orise_panel_data));
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

	err_device_put: platform_device_put(pdev);
	return ret;
}

static int __init mipi_orise_lcd_init(void)
{
	mipi_dsi_buf_alloc(&orise_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&orise_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_orise_lcd_init);
