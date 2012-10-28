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

#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/bitops.h>
#include <mach/camera.h>
#include <media/msm_camera.h>
#include "s5k4e1.h"

/* 16bit address - 8 bit context register structure */
#define Q8	0x00000100
#define Q10	0x00000400

/* MCLK */
#define S5K4E1_MASTER_CLK_RATE 24000000

/* AF Total steps parameters */
#define S5K4E1_TOTAL_STEPS_NEAR_TO_FAR	32
#if 1
/* HTC_START : full size preview from s5k4e1_reg.c */
#define S5K4E1_REG_PREV_FRAME_LEN_1	31
#define S5K4E1_REG_PREV_FRAME_LEN_2	32
#define S5K4E1_REG_PREV_LINE_LEN_1	33
#define S5K4E1_REG_PREV_LINE_LEN_2	34

/* HTC_START */
/* for reload frame length and line length of qtr or full size */
#define S5K4E1_REG_RECORD_FRAME_LEN_1	4
#define S5K4E1_REG_RECORD_FRAME_LEN_2	5
#define S5K4E1_REG_RECORD_LINE_LEN_1	6
#define S5K4E1_REG_RECORD_LINE_LEN_2	7
/* HTC_END */

#define S5K4E1_REG_SNAP_FRAME_LEN_1	31
#define S5K4E1_REG_SNAP_FRAME_LEN_2	32
#define S5K4E1_REG_SNAP_LINE_LEN_1	33
#define S5K4E1_REG_SNAP_LINE_LEN_2	34
/* HTC_END */
#else
#define S5K4E1_REG_PREV_FRAME_LEN_1	31
#define S5K4E1_REG_PREV_FRAME_LEN_2	32
#define S5K4E1_REG_PREV_LINE_LEN_1	33
#define S5K4E1_REG_PREV_LINE_LEN_2	34

#define S5K4E1_REG_SNAP_FRAME_LEN_1	15
#define S5K4E1_REG_SNAP_FRAME_LEN_2	16
#define  S5K4E1_REG_SNAP_LINE_LEN_1	17
#define S5K4E1_REG_SNAP_LINE_LEN_2	18
#endif

#define MSB                             1
#define LSB                             0

#define AF_5823_VCM_MODULE  /* For 5823 AF VCM Module */

#ifdef AF_5823_VCM_MODULE
#define S5K4E1_AF_I2C_ADDR 0x18
#define S5K4E1_VCM_CODE_MSB 0x04
#define S5K4E1_VCM_CODE_LSB 0x05
#define S5K4E1_MAX_FPS 30
#define S5K4E1_SW_DAMPING_STEP 10
uint16_t s5k4e1_pos_tbl[S5K4E1_TOTAL_STEPS_NEAR_TO_FAR + 1];
#endif

struct s5k4e1_work_t {
	struct work_struct work;
};

static struct s5k4e1_work_t *s5k4e1_sensorw;
static struct s5k4e1_work_t *s5k4e1_af_sensorw;
static struct i2c_client *s5k4e1_af_client;
static struct i2c_client *s5k4e1_client;

struct s5k4e1_ctrl_t {
	const struct  msm_camera_sensor_info *sensordata;

	uint32_t sensormode;
	uint32_t fps_divider;/* init to 1 * 0x00000400 */
	uint32_t pict_fps_divider;/* init to 1 * 0x00000400 */
	uint16_t fps;

	uint16_t curr_lens_pos;
	uint16_t curr_step_pos;
#ifdef AF_5823_VCM_MODULE
	uint16_t init_curr_lens_pos; //add it for 5823 AF VCM Module
#endif
	uint16_t my_reg_gain;
	uint32_t my_reg_line_count;
	uint16_t total_lines_per_frame;

	enum s5k4e1_resolution_t prev_res;
	enum s5k4e1_resolution_t pict_res;
	enum s5k4e1_resolution_t curr_res;
	enum s5k4e1_test_mode_t  set_test;
};

static bool CSI_CONFIG;
static struct s5k4e1_ctrl_t *s5k4e1_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(s5k4e1_wait_queue);
static DECLARE_WAIT_QUEUE_HEAD(s5k4e1_af_wait_queue);
DEFINE_MUTEX(s5k4e1_mut);

static uint16_t prev_line_length_pck;
static uint16_t prev_frame_length_lines;
static uint16_t snap_line_length_pck;
static uint16_t snap_frame_length_lines;

#if 1//for test i2c retry
#define MAX_I2C_RETRIES 20
static int i2c_transfer_retry(struct i2c_adapter *adap,
			struct i2c_msg *msgs,
			int len)
{
	int i2c_retry = 0;
	int ns; /* number sent */

	while (i2c_retry++ < MAX_I2C_RETRIES) {
		ns = i2c_transfer(adap, msgs, len);
		if (ns == len)
			break;
		pr_err("[CAM]%s: try %d/%d: i2c_transfer sent: %d, len %d\n",
			__func__,
			i2c_retry, MAX_I2C_RETRIES, ns, len);
		msleep(10);
	}

	return ns == len ? 0 : -EIO;
}
#endif

static int s5k4e1_i2c_rxdata(unsigned short saddr,
		unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
		{
			.addr  = saddr,
			.flags = 0,
			.len   = 1,
			.buf   = rxdata,
		},
		{
			.addr  = saddr,
			.flags = I2C_M_RD,
			.len   = 1,
			.buf   = rxdata,
		},
	};
#if 1 //for i2c retry
	if (i2c_transfer_retry(s5k4e1_client->adapter, msgs, 2) < 0) {
		printk("[CAM]s5k4e1_i2c_rxdata faild 0x%x\n", saddr);
		return -EIO;
	}
#else
	if (i2c_transfer(s5k4e1_client->adapter, msgs, 2) < 0) {
		pr_info("[CAM]s5k4e1_i2c_rxdata faild 0x%x\n", saddr);
		return -EIO;
	}
#endif
	return 0;
}

static int32_t s5k4e1_i2c_txdata(unsigned short saddr,
		unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};
	if (i2c_transfer(s5k4e1_client->adapter, msg, 1) < 0) {
		pr_info("[CAM]s5k4e1_i2c_txdata faild 0x%x\n", saddr);
		return -EIO;
	}

	return 0;
}

static int32_t s5k4e1_i2c_read(unsigned short raddr,
		unsigned short *rdata, int rlen)
{
	int32_t rc = 0;
	unsigned char buf[2];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));
	buf[0] = (raddr & 0xFF00) >> 8;
	buf[1] = (raddr & 0x00FF);
	rc = s5k4e1_i2c_rxdata(s5k4e1_client->addr, buf, rlen);
	if (rc < 0) {
		pr_info("[CAM]s5k4e1_i2c_read 0x%x failed!\n", raddr);
		return rc;
	}
	*rdata = (rlen == 2 ? buf[0] << 8 | buf[1] : buf[0]);
	CDBG("s5k4e1_i2c_read 0x%x val = 0x%x!\n", raddr, *rdata);

	return rc;
}

static int32_t s5k4e1_i2c_write_b_sensor(unsigned short waddr, uint8_t bdata)
{
	int32_t rc = -EFAULT;
	unsigned char buf[3];

	memset(buf, 0, sizeof(buf));
	buf[0] = (waddr & 0xFF00) >> 8;
	buf[1] = (waddr & 0x00FF);
	buf[2] = bdata;
	CDBG("i2c_write_b addr = 0x%x, val = 0x%x\n", waddr, bdata);
	rc = s5k4e1_i2c_txdata(s5k4e1_client->addr, buf, 3);
	if (rc < 0) {
		pr_info("[CAM]i2c_write_b failed, addr = 0x%x, val = 0x%x!\n",
				waddr, bdata);
	}
	return rc;
}

static int32_t s5k4e1_i2c_write_b_table(struct s5k4e1_i2c_reg_conf const
		*reg_conf_tbl, int num)
{
	int i;
	int32_t rc = -EIO;

	for (i = 0; i < num; i++) {
		rc = s5k4e1_i2c_write_b_sensor(reg_conf_tbl->waddr,
				reg_conf_tbl->wdata);
		if (rc < 0)
			break;
		reg_conf_tbl++;
	}
	return rc;
}

#ifndef AF_5823_VCM_MODULE //Mark it For 5820 AF VCM Module
static int32_t s5k4e1_af_i2c_txdata(unsigned short saddr,
		unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};
	if (i2c_transfer(s5k4e1_af_client->adapter, msg, 1) < 0) {
		pr_err("[CAM]s5k4e1_af_i2c_txdata faild 0x%x\n", saddr);
		return -EIO;
	}

	return 0;
}

static int32_t s5k4e1_af_i2c_write_b_sensor(uint8_t waddr, uint8_t bdata)
{
	int32_t rc = -EFAULT;
	unsigned char buf[2];

	memset(buf, 0, sizeof(buf));
	buf[0] = waddr;
	buf[1] = bdata;
	CDBG("i2c_write_b addr = 0x%x, val = 0x%x\n", waddr, bdata);
#if 0
	rc = s5k4e1_af_i2c_txdata(s5k4e1_af_client->addr << 1, buf, 2);
#else
	rc = s5k4e1_af_i2c_txdata(s5k4e1_af_client->addr , buf, 2); /* HTC */
#endif

	if (rc < 0) {
		pr_err("[CAM]af_i2c_write_b failed, addr = 0x%x, val = 0x%x!\n",
				waddr, bdata);
	}
	return rc;
}
#endif
static void s5k4e1_start_stream(void)
{
	s5k4e1_i2c_write_b_sensor(0x0100, 0x01);/* streaming on */
}

static void s5k4e1_stop_stream(void)
{
	s5k4e1_i2c_write_b_sensor(0x0100, 0x00);/* streaming off */
}

static void s5k4e1_group_hold_on(void)
{
	s5k4e1_i2c_write_b_sensor(0x0104, 0x01);
}

static void s5k4e1_group_hold_off(void)
{
	s5k4e1_i2c_write_b_sensor(0x0104, 0x0);
}

static void s5k4e1_get_pict_fps(uint16_t fps, uint16_t *pfps)
{
	/* input fps is preview fps in Q8 format */
	uint32_t divider, d1, d2;

	d1 = (prev_frame_length_lines * 0x00000400) / snap_frame_length_lines;
	d2 = (prev_line_length_pck * 0x00000400) / snap_line_length_pck;
	divider = (d1 * d2) / 0x400;

	/*Verify PCLK settings and frame sizes.*/
	*pfps = (uint16_t) (fps * divider / 0x400);
}

static uint16_t s5k4e1_get_prev_lines_pf(void)
{
	if (s5k4e1_ctrl->prev_res == QTR_SIZE)
		return prev_frame_length_lines;
	else
		return snap_frame_length_lines;
}

static uint16_t s5k4e1_get_prev_pixels_pl(void)
{
	if (s5k4e1_ctrl->prev_res == QTR_SIZE)
		return prev_line_length_pck;
	else
		return snap_line_length_pck;
}

static uint16_t s5k4e1_get_pict_lines_pf(void)
{
	if (s5k4e1_ctrl->pict_res == QTR_SIZE)
		return prev_frame_length_lines;
	else
		return snap_frame_length_lines;
}

static uint16_t s5k4e1_get_pict_pixels_pl(void)
{
	if (s5k4e1_ctrl->pict_res == QTR_SIZE)
		return prev_line_length_pck;
	else
		return snap_line_length_pck;
}

static uint32_t s5k4e1_get_pict_max_exp_lc(void)
{
	return snap_frame_length_lines * 24;
}

static int32_t s5k4e1_write_exp_gain(uint16_t gain, uint32_t line);
static int32_t s5k4e1_set_fps(struct fps_cfg   *fps)
{
	uint16_t total_lines_per_frame;
	int32_t rc = 0;
	uint32_t pre_fps = s5k4e1_ctrl->fps_divider;

	s5k4e1_ctrl->fps_divider = fps->fps_div;
	s5k4e1_ctrl->pict_fps_divider = fps->pict_fps_div;
	s5k4e1_ctrl->fps = fps->f_mult;

	if (s5k4e1_ctrl->sensormode == SENSOR_PREVIEW_MODE) {
		total_lines_per_frame = (uint16_t)
		((prev_frame_length_lines * s5k4e1_ctrl->fps_divider) / 0x400);
	} else {
		total_lines_per_frame = (uint16_t)
		((snap_frame_length_lines * s5k4e1_ctrl->fps_divider) / 0x400);
	}
/* HTC_START Qingyuan_li 20120220 */
/* sync from s5k4e5 driver */
	if (s5k4e1_ctrl->sensormode == SENSOR_PREVIEW_MODE &&
		(s5k4e1_ctrl->my_reg_gain != 0 || s5k4e1_ctrl->my_reg_line_count != 0)) {
		rc =
			s5k4e1_write_exp_gain(s5k4e1_ctrl->my_reg_gain,
				s5k4e1_ctrl->my_reg_line_count * pre_fps / s5k4e1_ctrl->fps_divider);
	}
/* HTC_END */
	return rc;
}

static inline uint8_t s5k4e1_byte(uint16_t word, uint8_t offset)
{
	return word >> (offset * BITS_PER_BYTE);
}

#if 0
static int32_t s5k4e1_write_exp_gain(uint16_t gain, uint32_t line)
{
	uint16_t max_legal_gain = 0x0200;
	int32_t rc = 0;
	static uint32_t fl_lines;

	if (gain > max_legal_gain) {
		pr_debug("[CAM]Max legal gain Line:%d\n", __LINE__);
		gain = max_legal_gain;
	}

/*HTC_START Qingyuan_li 20120213 */
/* hold on before setting exposure parameter */
	s5k4e1_group_hold_on();
/*HTC_END  */
	/* Analogue Gain */
	s5k4e1_i2c_write_b_sensor(0x0204, s5k4e1_byte(gain, MSB));
	s5k4e1_i2c_write_b_sensor(0x0205, s5k4e1_byte(gain, LSB));

	if (line > (prev_frame_length_lines - 4)) {
		fl_lines = line+4;
		s5k4e1_i2c_write_b_sensor(0x0340, s5k4e1_byte(fl_lines, MSB));
		s5k4e1_i2c_write_b_sensor(0x0341, s5k4e1_byte(fl_lines, LSB));
		/* Coarse Integration Time */
		s5k4e1_i2c_write_b_sensor(0x0202, s5k4e1_byte(line, MSB));
		s5k4e1_i2c_write_b_sensor(0x0203, s5k4e1_byte(line, LSB));
	} else if (line < (fl_lines - 4)) {
		fl_lines = line+4;
		if (fl_lines < prev_frame_length_lines)
			fl_lines = prev_frame_length_lines;

		/* Coarse Integration Time */
		s5k4e1_i2c_write_b_sensor(0x0202, s5k4e1_byte(line, MSB));
		s5k4e1_i2c_write_b_sensor(0x0203, s5k4e1_byte(line, LSB));
		s5k4e1_i2c_write_b_sensor(0x0340, s5k4e1_byte(fl_lines, MSB));
		s5k4e1_i2c_write_b_sensor(0x0341, s5k4e1_byte(fl_lines, LSB));
	} else {
		fl_lines = line+4;
		/* Coarse Integration Time */
		s5k4e1_i2c_write_b_sensor(0x0202, s5k4e1_byte(line, MSB));
		s5k4e1_i2c_write_b_sensor(0x0203, s5k4e1_byte(line, LSB));
	}

/*HTC_START Qingyuan_li 20120213 */
	s5k4e1_group_hold_off();
/*HTC_END  */
	return rc;
}
#endif

/* HTC_START Qingyuan_li 20120220 */
/* sync from s5k4e5 driver */
#if 1

#define REG_LINE_LENGTH_PCK_MSB 0x0342
#define REG_LINE_LENGTH_PCK_LSB 0x0343
#define REG_ANALOGUE_GAIN_CODE_GLOBAL_MSB 0x0204
#define REG_ANALOGUE_GAIN_CODE_GLOBAL_LSB 0x0205
#define REG_COARSE_INTEGRATION_TIME_MSB 0x0202
#define REG_COARSE_INTEGRATION_TIME_LSB 0x0203

#define S5K4E1_REG_GROUP_PARAMETER_HOLD 0x0104
#define S5K4E1_GROUP_PARAMETER_HOLD 0x01
#define S5K4E1_GROUP_PARAMETER_UNHOLD 0x00

#define SENSOR_HRZ_FULL_BLK_PIXELS 192 /* Mu Lee 0706 575 */
#define SENSOR_VER_FULL_BLK_LINES 71 /* Mu Lee 0706 582 *//* 12 Aaron */
#define SENSOR_HRZ_QTR_BLK_PIXELS 2048 /*1434*/
#define SENSOR_VER_QTR_BLK_LINES 237 /*12*/ /* 296 *//* 12 Aaron *//*Mu Lee 24fps*/

/* CAMIF output resolutions */
/* 816x612, 24MHz MCLK 96MHz PCLK */
#define SENSOR_FULL_SIZE_WIDTH 2608
#define SENSOR_FULL_SIZE_HEIGHT 1960

#define SENSOR_QTR_SIZE_WIDTH 1304
#define SENSOR_QTR_SIZE_HEIGHT 980
static int32_t s5k4e1_write_exp_gain
  (uint16_t gain, uint32_t line)
{
	int32_t rc = 0;

	uint16_t max_legal_gain = 0x0200;
	uint32_t ll_ratio; /* Q10 */
	uint32_t ll_pck, fl_lines;
	uint16_t offset = 8; /* 4; */     /* kipper */
	uint32_t gain_msb, gain_lsb;
	uint32_t intg_t_msb, intg_t_lsb;
	uint32_t ll_pck_msb, ll_pck_lsb;
	struct s5k4e1_i2c_reg_conf tbl[3];

	CDBG("Line:%d s5k4e1_write_exp_gain \n", __LINE__);

	/* sync from SAGA to avoid frame is black*/
	if ((gain == 0) || (line == 0)) {
		pr_info("[CAM]s5k4e1gx_write_exp_gain: gain=0 or line=0 (gain=%d,line=%d)\n", gain, line);
		return rc;
	}

	if (s5k4e1_ctrl->sensormode == SENSOR_PREVIEW_MODE) {

		s5k4e1_ctrl->my_reg_gain = gain;
		s5k4e1_ctrl->my_reg_line_count = (uint16_t)line;
		if (s5k4e1_ctrl->prev_res == QTR_SIZE) {
			/* s5k4e1_ctrl->prev_res == QTR_SIZE */
			fl_lines = SENSOR_QTR_SIZE_HEIGHT +
				SENSOR_VER_QTR_BLK_LINES;

			ll_pck = SENSOR_QTR_SIZE_WIDTH +
				SENSOR_HRZ_QTR_BLK_PIXELS;
		} else {
		/* s5k4e1_ctrl->prev_res == FULL_SIZE */
			CDBG("-----full size ------\n");
			fl_lines = SENSOR_FULL_SIZE_HEIGHT +
				SENSOR_VER_FULL_BLK_LINES;

			ll_pck = SENSOR_FULL_SIZE_WIDTH +
				SENSOR_HRZ_FULL_BLK_PIXELS;
		}
	} else {

		fl_lines = SENSOR_FULL_SIZE_HEIGHT +
			SENSOR_VER_FULL_BLK_LINES;

		ll_pck = SENSOR_FULL_SIZE_WIDTH +
			SENSOR_HRZ_FULL_BLK_PIXELS;
	}

	if (gain > max_legal_gain)
		gain = max_legal_gain;

	/* in Q10 */
	line = (line * 0x400);/* s5k4e1_ctrl->fps_divider); */

	CDBG("s5k4e1_ctrl->fps_divider = %d\n", s5k4e1_ctrl->fps_divider);
	CDBG("fl_lines = %d\n", fl_lines);
	CDBG("line = %d\n", line);

	if ((fl_lines - offset) < (line / 0x400))
		ll_ratio = (line / (fl_lines - offset));
	else
		ll_ratio = 0x400;
	 CDBG("ll_ratio = %d\n", ll_ratio);

	/* update gain registers */
	CDBG("gain = %d\n", gain);
	gain_msb = (gain & 0xFF00) >> 8;
	gain_lsb = gain & 0x00FF;
	tbl[0].waddr = S5K4E1_REG_GROUP_PARAMETER_HOLD;
	tbl[0].wdata = S5K4E1_GROUP_PARAMETER_HOLD;
	tbl[1].waddr = REG_ANALOGUE_GAIN_CODE_GLOBAL_MSB;
	tbl[1].wdata = gain_msb;
	tbl[2].waddr = REG_ANALOGUE_GAIN_CODE_GLOBAL_LSB;
	tbl[2].wdata = gain_lsb;
	rc = s5k4e1_i2c_write_b_table(&tbl[0], ARRAY_SIZE(tbl));
	if (rc < 0)
		goto write_gain_done;

	ll_pck = ll_pck * ll_ratio / 0x400 * s5k4e1_ctrl->fps_divider;
	CDBG("ll_pck/0x400 = %d\n", ll_pck / 0x400);
	ll_pck_msb = ((ll_pck / 0x400) & 0xFF00) >> 8;
	ll_pck_lsb = (ll_pck / 0x400) & 0x00FF;
	tbl[0].waddr = REG_LINE_LENGTH_PCK_MSB;
	tbl[0].wdata = ll_pck_msb;
	tbl[1].waddr = REG_LINE_LENGTH_PCK_LSB;
	tbl[1].wdata = ll_pck_lsb;
	rc = s5k4e1_i2c_write_b_table(&tbl[0], ARRAY_SIZE(tbl)-1);
	if (rc < 0)
		goto write_gain_done;

	line = line / ll_ratio;
	CDBG("line = %d\n", line);
	intg_t_msb = (line & 0xFF00) >> 8;
	intg_t_lsb = (line & 0x00FF);
	tbl[0].waddr = REG_COARSE_INTEGRATION_TIME_MSB;
	tbl[0].wdata = intg_t_msb;
	tbl[1].waddr = REG_COARSE_INTEGRATION_TIME_LSB;
	tbl[1].wdata = intg_t_lsb;
	tbl[2].waddr = S5K4E1_REG_GROUP_PARAMETER_HOLD;
	tbl[2].wdata = S5K4E1_GROUP_PARAMETER_UNHOLD;
	rc = s5k4e1_i2c_write_b_table(&tbl[0], ARRAY_SIZE(tbl));

write_gain_done:
	return rc;
}
#endif
/* HTC_END */

static int32_t s5k4e1_set_pict_exp_gain(uint16_t gain, uint32_t line)
{
	uint16_t max_legal_gain = 0x0200;
	uint16_t min_ll_pck = 0x0AB2;
	uint32_t ll_pck, fl_lines;
	uint32_t ll_ratio;
	int32_t rc = 0;
	uint8_t gain_msb, gain_lsb;
	uint8_t intg_time_msb, intg_time_lsb;
	uint8_t ll_pck_msb, ll_pck_lsb;

	if (gain > max_legal_gain) {
		pr_debug("[CAM]Max legal gain Line:%d\n", __LINE__);
		gain = max_legal_gain;
	}

	pr_debug("[CAM]s5k4e1_write_exp_gain : gain = %d line = %d\n", gain, line);
	line = (uint32_t) (line * s5k4e1_ctrl->pict_fps_divider);
	fl_lines = snap_frame_length_lines;
	ll_pck = snap_line_length_pck;

	if (fl_lines < (line / 0x400))
		ll_ratio = (line / (fl_lines - 4));
	else
		ll_ratio = 0x400;

	ll_pck = ll_pck * ll_ratio / 0x400;
	line = line / ll_ratio;
	if (ll_pck < min_ll_pck)
		ll_pck = min_ll_pck;

	gain_msb = (uint8_t) ((gain & 0xFF00) >> 8);
	gain_lsb = (uint8_t) (gain & 0x00FF);

	intg_time_msb = (uint8_t) ((line & 0xFF00) >> 8);
	intg_time_lsb = (uint8_t) (line & 0x00FF);

	ll_pck_msb = (uint8_t) ((ll_pck & 0xFF00) >> 8);
	ll_pck_lsb = (uint8_t) (ll_pck & 0x00FF);

	s5k4e1_group_hold_on();
	s5k4e1_i2c_write_b_sensor(0x0204, gain_msb); /* Analogue Gain */
	s5k4e1_i2c_write_b_sensor(0x0205, gain_lsb);

	s5k4e1_i2c_write_b_sensor(0x0342, ll_pck_msb);
	s5k4e1_i2c_write_b_sensor(0x0343, ll_pck_lsb);

	/* Coarse Integration Time */
	s5k4e1_i2c_write_b_sensor(0x0202, intg_time_msb);
	s5k4e1_i2c_write_b_sensor(0x0203, intg_time_lsb);
	s5k4e1_group_hold_off();

	return rc;
}

#ifdef AF_5823_VCM_MODULE  //for 5823 AF VCM Module
static void s5k4e1_setup_af_tbl(void)
{
  uint32_t i;
#ifndef CONFIG_RAWCHIP
  uint16_t s5k4e1_nl_region_boundary1 = 3;
  uint16_t s5k4e1_nl_region_boundary2 = 5;
  uint16_t s5k4e1_nl_region_code_per_step1 = 40;
  uint16_t s5k4e1_nl_region_code_per_step2 = 20;
  uint16_t s5k4e1_l_region_code_per_step = 16;
#endif
  s5k4e1_pos_tbl[0] = 0;

  for (i = 1; i <= S5K4E1_TOTAL_STEPS_NEAR_TO_FAR; i++) {
#ifndef CONFIG_RAWCHIP
    if (i <= s5k4e1_nl_region_boundary1)
      s5k4e1_pos_tbl[i] = s5k4e1_pos_tbl[i-1] +
      s5k4e1_nl_region_code_per_step1;
    else if (i <= s5k4e1_nl_region_boundary2)
      s5k4e1_pos_tbl[i] = s5k4e1_pos_tbl[i-1] +
      s5k4e1_nl_region_code_per_step2;
    else
      s5k4e1_pos_tbl[i] = s5k4e1_pos_tbl[i-1] +
      s5k4e1_l_region_code_per_step;
#else
	s5k4e1_pos_tbl[i] =  s5k4e1_pos_tbl[i-1] + 4;
#endif
  }
}

int32_t s5k4e1_go_to_position(uint32_t lens_pos, uint8_t mask)
{
	int32_t rc = 0;
	unsigned char buf[2];
	uint8_t vcm_code_msb, vcm_code_lsb;

	vcm_code_msb = (lens_pos >> 8) & 0x3;
	vcm_code_lsb = lens_pos & 0xFF;

	buf[0] = S5K4E1_VCM_CODE_MSB;
	buf[1] = vcm_code_msb;

	rc = s5k4e1_i2c_txdata(S5K4E1_AF_I2C_ADDR >> 1, buf, 2);

	if (rc < 0)
		pr_err("[CAM]i2c_write failed, saddr = 0x%x addr = 0x%x, val =0x%x!\n", S5K4E1_AF_I2C_ADDR >> 1, buf[0], buf[1]);

	buf[0] = S5K4E1_VCM_CODE_LSB;
	buf[1] = vcm_code_lsb;

	rc = s5k4e1_i2c_txdata(S5K4E1_AF_I2C_ADDR >> 1, buf, 2);

	if (rc < 0)
		pr_err("[CAM]i2c_write failed, saddr = 0x%x addr = 0x%x, val =0x%x!\n", S5K4E1_AF_I2C_ADDR >> 1, buf[0], buf[1]);

	return rc;
}
#endif

static int32_t s5k4e1_move_focus(int direction, int32_t num_steps)
{
#ifdef AF_5823_VCM_MODULE
  uint16_t s5k4e1_sw_damping_time_wait = 1;
  uint16_t s5k4e1_damping_threshold = 10;
  uint8_t s5k4e1_mode_mask = 0x02;
  int16_t step_direction;
  int16_t curr_lens_pos;
  int16_t curr_step_pos;
  int16_t dest_lens_pos;
  int16_t dest_step_pos;
  int16_t target_dist;
  int16_t small_step;
  int16_t next_lens_pos;
  int16_t time_wait_per_step;
  int32_t rc = 0, time_wait;
  int8_t s5k4e1_sw_damping_required = 0;
  uint16_t s5k4e1_max_fps_val;

  if (num_steps > S5K4E1_TOTAL_STEPS_NEAR_TO_FAR)
      num_steps = S5K4E1_TOTAL_STEPS_NEAR_TO_FAR;
  else if (num_steps == 0)
      return -EINVAL;

  if (direction == MOVE_NEAR)
      step_direction = 1;
  else if (direction == MOVE_FAR)
      step_direction = -1;
  else
      return -EINVAL;

  /* need to decide about default position and power supplied
   * at start up and reset */
  curr_lens_pos = s5k4e1_ctrl->curr_lens_pos;
  curr_step_pos = s5k4e1_ctrl->curr_step_pos;

  if (curr_lens_pos < s5k4e1_ctrl->init_curr_lens_pos)
      curr_lens_pos = s5k4e1_ctrl->init_curr_lens_pos;

  dest_step_pos = curr_step_pos + (step_direction * num_steps);

  if (dest_step_pos < 0)
      dest_step_pos = 0;
  else if (dest_step_pos > S5K4E1_TOTAL_STEPS_NEAR_TO_FAR)
      dest_step_pos = S5K4E1_TOTAL_STEPS_NEAR_TO_FAR;

#ifdef CONFIG_RAWCHIP
	cur_step_pos = dest_step_pos;
#endif

  if (dest_step_pos == s5k4e1_ctrl->curr_step_pos)
      return rc;

  dest_lens_pos = s5k4e1_pos_tbl[dest_step_pos];

/* For Af measureing for optical*/
#if 0
  dest_lens_pos = AF_START_POSITION+(dest_step_pos-1)*AF_STEP_SIZE;
  pr_err("[CAM] [%d] curr_lens_pos = %d!!!\n",dest_step_pos, dest_lens_pos);
#endif
  target_dist = step_direction * (dest_lens_pos - curr_lens_pos);

    s5k4e1_max_fps_val = S5K4E1_MAX_FPS;

  /* HW damping */
  if (step_direction < 0
    && target_dist >= s5k4e1_pos_tbl[s5k4e1_damping_threshold]) {
    s5k4e1_sw_damping_required = 1;
    time_wait = 1000000
      / s5k4e1_max_fps_val
      - S5K4E1_SW_DAMPING_STEP * s5k4e1_sw_damping_time_wait * 1000;
  } else
    time_wait = 1000000 / s5k4e1_max_fps_val;

  time_wait_per_step = (int16_t) (time_wait / target_dist);

  if (time_wait_per_step >= 800)
    /* ~800 */
    s5k4e1_mode_mask = 0x5;
  else if (time_wait_per_step >= 400)
    /* ~400 */
    s5k4e1_mode_mask = 0x4;
  else if (time_wait_per_step >= 200)
    /* 200~400 */
    s5k4e1_mode_mask = 0x3;
  else if (time_wait_per_step >= 100)
    /* 100~200 */
    s5k4e1_mode_mask = 0x2;
  else if (time_wait_per_step >= 50)
    /* 50~100 */
    s5k4e1_mode_mask = 0x1;
  else {
    if (time_wait >= 17600)
      s5k4e1_mode_mask = 0x0D;
    else if (time_wait >= 8800)
      s5k4e1_mode_mask = 0x0C;
    else if (time_wait >= 4400)
      s5k4e1_mode_mask = 0x0B;
    else if (time_wait >= 2200)
      s5k4e1_mode_mask = 0x0A;
    else
      s5k4e1_mode_mask = 0x09;
  }

  if (s5k4e1_sw_damping_required) {
    small_step = (uint16_t) target_dist / S5K4E1_SW_DAMPING_STEP;
    if ((target_dist % S5K4E1_SW_DAMPING_STEP) != 0)
      small_step = small_step + 1;

    for (next_lens_pos = curr_lens_pos + (step_direction*small_step);
      (step_direction*next_lens_pos) <= (step_direction*dest_lens_pos);
      next_lens_pos += (step_direction*small_step)) {
      rc = s5k4e1_go_to_position(next_lens_pos, s5k4e1_mode_mask);
      if (rc < 0) {
      pr_err("[CAM]s5k4e1_go_to_position Failed in Move Focus!!!\n");
      return rc;
      }
      curr_lens_pos = next_lens_pos;
      mdelay(s5k4e1_sw_damping_time_wait);
    }

    if (curr_lens_pos != dest_lens_pos) {
      rc = s5k4e1_go_to_position(dest_lens_pos, s5k4e1_mode_mask);
      if (rc < 0) {
      pr_err("[CAM]s5k4e1_go_to_position Failed in Move Focus!!!\n");
      return rc;
      }
      mdelay(s5k4e1_sw_damping_time_wait);
    }
  } else {
    rc = s5k4e1_go_to_position(dest_lens_pos, s5k4e1_mode_mask);
    if (rc < 0) {
      pr_err("[CAM]s5k4e1_go_to_position Failed in Move Focus!!!\n");
      return rc;
    }
  }

  s5k4e1_ctrl->curr_lens_pos = dest_lens_pos;
  s5k4e1_ctrl->curr_step_pos = dest_step_pos;

#else /* For 5820 AF VCM Module */
	int16_t step_direction, actual_step, next_position;
	uint8_t code_val_msb, code_val_lsb;

	if (direction == MOVE_NEAR)
		step_direction = 16;
	else
		step_direction = -16;

	actual_step = (int16_t) (step_direction * num_steps);
	next_position = (int16_t) (s5k4e1_ctrl->curr_lens_pos + actual_step);

	if (next_position > 1023)
		next_position = 1023;
	else if (next_position < 0)
		next_position = 0;

	code_val_msb = next_position >> 4;
	code_val_lsb = (next_position & 0x000F) << 4;

	if (s5k4e1_af_i2c_write_b_sensor(code_val_msb, code_val_lsb) < 0) {
		pr_err("[CAM] move_focus failed at line %d ...\n", __LINE__);
		return -EBUSY;
	}

	s5k4e1_ctrl->curr_lens_pos = next_position;
#endif

	return 0;
}

static int32_t s5k4e1_set_default_focus(void)
{
	int32_t rc = 0;

#ifdef AF_5823_VCM_MODULE
  if (s5k4e1_ctrl->curr_step_pos != 0) {
    rc = s5k4e1_move_focus(MOVE_FAR, s5k4e1_ctrl->curr_step_pos);
    if (rc < 0) {
      pr_err("[CAM]s5k4e1_set_default_focus Failed!!!\n");
      return rc;
    }
  } else {
    rc = s5k4e1_go_to_position(0, 0x02);
    if (rc < 0) {
      pr_err("[CAM]s5k4e1_go_to_position Failed!!!\n");
      return rc;
    }
  }

  s5k4e1_ctrl->curr_lens_pos = 0;
  s5k4e1_ctrl->init_curr_lens_pos = 0;
  s5k4e1_ctrl->curr_step_pos = 0;
#else /* for 5820 AF VCM Module */
  if (s5k4e1_ctrl->curr_step_pos != 0) {
    rc = s5k4e1_move_focus(MOVE_FAR, s5k4e1_ctrl->curr_step_pos);
  } else {
    s5k4e1_af_i2c_write_b_sensor(0x00, 0x00);
  }

	s5k4e1_ctrl->curr_lens_pos = 0;
	s5k4e1_ctrl->curr_step_pos = 0;
#endif

  return rc;
}

static int32_t s5k4e1_test(enum s5k4e1_test_mode_t mo)
{
	int32_t rc = 0;

	if (mo != TEST_OFF)
		rc = s5k4e1_i2c_write_b_sensor(0x0601, (uint8_t) mo);

	return rc;
}

static void s5k4e1_reset_sensor(void)
{
	s5k4e1_i2c_write_b_sensor(0x103, 0x1);
}

static int32_t s5k4e1_sensor_setting(int update_type, int rt)
{

	int32_t rc = 0;
	struct msm_camera_csi_params s5k4e1_csi_params;
/* HTC_START */
	if (rt == RES_CAPTURE && update_type == UPDATE_PERIODIC && s5k4e1_ctrl->sensordata->full_size_preview){
		pr_info("[CAM]s5k4e1_sensor_setting: Full size preview (RES_CAPTURE: return), Not set setting\n");
		return rc;
	}
/* HTC_END */
	s5k4e1_stop_stream();
	msleep(30);

	if (update_type == REG_INIT) {
		/* HTC_START */

		if (s5k4e1_ctrl->sensordata->csi_if)
			s5k4e1_i2c_write_b_sensor(0x3030,0x06);
		else
			s5k4e1_reset_sensor();
		/* HTC_END */
		s5k4e1_i2c_write_b_table(s5k4e1_regs.reg_mipi,
				s5k4e1_regs.reg_mipi_size);
#if 0 /* move to UPDATE_PERIODIC due to two settings of qtr and full size*/
		s5k4e1_i2c_write_b_table(s5k4e1_regs.rec_settings,
				s5k4e1_regs.rec_size);
#endif
		s5k4e1_i2c_write_b_table(s5k4e1_regs.reg_pll_p,
				s5k4e1_regs.reg_pll_p_size);
		CSI_CONFIG = 0;
	} else if (update_type == UPDATE_PERIODIC) {
/* HTC_START */
		if (rt == RES_RECORD){
			pr_info("[CAM] set record qtr size recommend setting\n");
			s5k4e1_i2c_write_b_table(s5k4e1_regs.rec_record_settings, // qtr size preview setting
					s5k4e1_regs.rec_record_settings_size);
		} else {
			pr_info("[CAM] set preview full size recommend setting\n");
			s5k4e1_i2c_write_b_table(s5k4e1_regs.rec_settings, // full size preview setting
					s5k4e1_regs.rec_size);
		}
/* HTC_END */

		if (rt == RES_PREVIEW){
			pr_info("[CAM] set preview full size setting\n");
			s5k4e1_i2c_write_b_table(s5k4e1_regs.reg_prev, // full size preview setting
					s5k4e1_regs.reg_prev_size);
/* HTC_START */
			/* for reload frame length and line length of qtr or full size */
	        prev_frame_length_lines =
	        ((s5k4e1_regs.reg_prev[S5K4E1_REG_PREV_FRAME_LEN_1].wdata << 8) |
		        s5k4e1_regs.reg_prev[S5K4E1_REG_PREV_FRAME_LEN_2].wdata);

	        prev_line_length_pck =
	        (s5k4e1_regs.reg_prev[S5K4E1_REG_PREV_LINE_LEN_1].wdata << 8) |
		        s5k4e1_regs.reg_prev[S5K4E1_REG_PREV_LINE_LEN_2].wdata;
/* HTC_END */
		} else if (rt == RES_RECORD){
			pr_info("[CAM] set record qtr size setting\n");
			s5k4e1_i2c_write_b_table(s5k4e1_regs.reg_record, //  size preview setting
					s5k4e1_regs.reg_record_size);
/* HTC_START */
			/* for reload frame length and line length of qtr or full size */
	        prev_frame_length_lines =
	        ((s5k4e1_regs.reg_record[S5K4E1_REG_RECORD_FRAME_LEN_1].wdata << 8) |
		        s5k4e1_regs.reg_record[S5K4E1_REG_RECORD_FRAME_LEN_2].wdata);

	        prev_line_length_pck =
	        (s5k4e1_regs.reg_record[S5K4E1_REG_RECORD_LINE_LEN_1].wdata << 8) |
		        s5k4e1_regs.reg_record[S5K4E1_REG_RECORD_LINE_LEN_2].wdata;
/* HTC_END */
		} else {
			s5k4e1_i2c_write_b_table(s5k4e1_regs.reg_snap,
					s5k4e1_regs.reg_snap_size);
		}

		msleep(20);
		if (!CSI_CONFIG) {
			msm_camio_vfe_clk_rate_set(192000000);
			s5k4e1_csi_params.data_format = CSI_10BIT;
			s5k4e1_csi_params.lane_cnt = 2;
			s5k4e1_csi_params.lane_assign = 0xe4;
			s5k4e1_csi_params.dpcm_scheme = 0;
			s5k4e1_csi_params.settle_cnt = 24;
/* HTC_START */
			/* sync from s5k4e5yx sensor */
			s5k4e1_csi_params.hs_impedence = 0x0F;
			s5k4e1_csi_params.mipi_driving_strength = 0;
/* HTC_END */
			rc = msm_camio_csi_config(&s5k4e1_csi_params);
			msleep(20);
			CSI_CONFIG = 1;
		}

		s5k4e1_start_stream();
		msleep(30);
	}
	return rc;
}

static int32_t s5k4e1_video_config(int mode)
{

	int32_t rc = 0;
	int rt;
	CDBG("video config  curr_res %d prev_res=%d\n",s5k4e1_ctrl->curr_res,s5k4e1_ctrl->prev_res);
	/* change sensor resolution if needed */

/* HTC_START Qingyuan_li 20120320 */

	/* setting will be set when change sensor output mode. mode:1.full size 2. qtr size*/
	if (s5k4e1_ctrl->curr_res != s5k4e1_ctrl->prev_res) {
		if (s5k4e1_ctrl->prev_res == QTR_SIZE)
			rt = RES_RECORD;
		else {
			if(s5k4e1_ctrl->sensordata->full_size_preview)
				rt = RES_PREVIEW;
			else
				rt = RES_CAPTURE;
		}
		if (s5k4e1_sensor_setting(UPDATE_PERIODIC, rt) < 0)
			return rc;
		if (s5k4e1_ctrl->set_test) {
			if (s5k4e1_test(s5k4e1_ctrl->set_test) < 0)
				return  rc;
		}
	}
/* HTC_END */
	s5k4e1_ctrl->curr_res = s5k4e1_ctrl->prev_res;
	s5k4e1_ctrl->sensormode = mode;
	return rc;
}

static int32_t s5k4e1_snapshot_config(int mode)
{
	int32_t rc = 0;
	int rt;
	if(s5k4e1_ctrl->sensordata->full_size_preview){
		pr_info("[CAM] s5k4e1_snapshot_config: full preview , return 0\n");
		return rc;
	}

	/*change sensor resolution if needed */
	if (s5k4e1_ctrl->curr_res != s5k4e1_ctrl->pict_res) {
		if (s5k4e1_ctrl->pict_res == QTR_SIZE)
			rt = RES_PREVIEW;
		else
			rt = RES_CAPTURE;
		if (s5k4e1_sensor_setting(UPDATE_PERIODIC, rt) < 0)
			return rc;
	}

	s5k4e1_ctrl->curr_res = s5k4e1_ctrl->pict_res;
	s5k4e1_ctrl->sensormode = mode;
	return rc;
}

static int32_t s5k4e1_raw_snapshot_config(int mode)
{
	int32_t rc = 0;
	int rt;
	if(s5k4e1_ctrl->sensordata->full_size_preview){
		pr_info("[CAM] s5k4e1_raw_snapshot_config: full preview , return 0\n");
		return rc;
	}

	/* change sensor resolution if needed */
	if (s5k4e1_ctrl->curr_res != s5k4e1_ctrl->pict_res) {
		if (s5k4e1_ctrl->pict_res == QTR_SIZE)
			rt = RES_PREVIEW;
		else
			rt = RES_CAPTURE;
		if (s5k4e1_sensor_setting(UPDATE_PERIODIC, rt) < 0)
			return rc;
	}

	s5k4e1_ctrl->curr_res = s5k4e1_ctrl->pict_res;
	s5k4e1_ctrl->sensormode = mode;
	return rc;
}

static int32_t s5k4e1_set_sensor_mode(int mode,
		int res)
{
	int32_t rc = 0;

	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		s5k4e1_ctrl->prev_res = res; // FULL_SIZE, QTR_SIZE
		rc = s5k4e1_video_config(mode);
		break;
	case SENSOR_SNAPSHOT_MODE:
		rc = s5k4e1_snapshot_config(mode);
		break;
	case SENSOR_RAW_SNAPSHOT_MODE:
		rc = s5k4e1_raw_snapshot_config(mode);
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}

static int32_t s5k4e1_power_down(void)
{
	mdelay(1);
	s5k4e1_stop_stream();
/* HTC_START */
	mdelay(110);
/*HTC_END */
	return 0;
}

#if 0
static int s5k4e1_probe_init_done(const struct msm_camera_sensor_info *data)
{
	pr_info("probe done\n");
	gpio_free(data->sensor_reset);
	return 0;
}
#endif

static int s5k4e1_probe_init_sensor(const struct msm_camera_sensor_info *data)
{
	int32_t rc = 0;
	uint16_t regaddress1 = 0x0000;
	uint16_t regaddress2 = 0x0001;
	uint16_t chipid1 = 0;
	uint16_t chipid2 = 0;

	CDBG("%s: %d\n", __func__, __LINE__);
	CDBG(" s5k4e1_probe_init_sensor is called\n");

#if 1
	//gpio_set_value(data->sensor_reset, 0);
	//msleep(50);
	if(data)
		gpio_set_value(data->sensor_reset, 1);
#else
	rc = gpio_request(data->sensor_reset, "s5k4e1");
	CDBG(" s5k4e1_probe_init_sensor\n");
	if (!rc) {
		CDBG("sensor_reset = %d\n", rc);
		gpio_direction_output(data->sensor_reset, 0);
		msleep(50);
		gpio_set_value_cansleep(data->sensor_reset, 1);
		msleep(20);
	} else
		goto gpio_req_fail;
#endif

	msleep(2);

	s5k4e1_i2c_read(regaddress1, &chipid1, 1);
	if (chipid1 != 0x4E) {
		rc = -ENODEV;
		pr_info("[CAM]s5k4e1_probe_init_sensor fail chip id doesnot match\n");
		goto init_probe_fail;
	}

	s5k4e1_i2c_read(regaddress2, &chipid2 , 1);
	if (chipid2 != 0x10) {
		rc = -ENODEV;
		pr_info("[CAM]s5k4e1_probe_init_sensor fail chip id doesnot match\n");
		goto init_probe_fail;
	}

	CDBG("ID: %d\n", chipid1);
	CDBG("ID: %d\n", chipid2);

	return rc;

init_probe_fail:
	pr_info(" [CAM]s5k4e1_probe_init_sensor fails\n");
/* HTC START */
#if 1
	if(data)
		gpio_set_value(data->sensor_reset, 0);
#else
	gpio_set_value_cansleep(data->sensor_reset, 0);
	s5k4e1_probe_init_done(data);
#endif
/* HTC_END */

#if 0 /* Mark it due to this action is not done at probe stage */
	if (data->vcm_enable) {
		int ret = gpio_request(data->vcm_pwd, "s5k4e1_af");
		if (!ret) {
			gpio_direction_output(data->vcm_pwd, 0);
			msleep(20);
			gpio_free(data->vcm_pwd);
		}
	}
gpio_req_fail:
#endif

	return rc;
}

static int s5k4e1_i2c_read_fuseid(struct sensor_cfg_data *cdata)
{

	int32_t  rc;
	unsigned short i, R1, R2, R3;
	unsigned short  OTP[10] = {0};

	pr_info("[CAM]%s: sensor OTP information:\n", __func__);

	rc = s5k4e1_i2c_write_b_sensor(0x30F9, 0x0E);
	if (rc < 0)
		pr_info("[CAM]%s: i2c_write_b 0x30F9 fail\n", __func__);

	rc = s5k4e1_i2c_write_b_sensor(0x30FA, 0x0A);
	if (rc < 0)
		pr_info("[CAM]%s: i2c_write_b 0x30FA fail\n", __func__);

	rc = s5k4e1_i2c_write_b_sensor(0x30FB, 0x71);
	if (rc < 0)
		pr_info("[CAM]%s: i2c_write_b 0x30FB fail\n", __func__);

	rc = s5k4e1_i2c_write_b_sensor(0x30FB, 0x70);
	if (rc < 0)
		pr_info("[CAM]%s: i2c_write_b 0x30FB fail\n", __func__);

	mdelay(4);

	for (i = 0; i < 10; i++) {
		rc = s5k4e1_i2c_write_b_sensor(0x310C, i);
		if (rc < 0)
			pr_info("[CAM]%s: i2c_write_b 0x310C fail\n", __func__);
		rc = s5k4e1_i2c_read(0x310F, &R1, 1);
		if (rc < 0)
			pr_info("[CAM]%s: i2c_read_b 0x310F fail\n", __func__);
		rc = s5k4e1_i2c_read(0x310E, &R2, 1);
		if (rc < 0)
			pr_info("[CAM]%s: i2c_read_b 0x310E fail\n", __func__);
		rc = s5k4e1_i2c_read(0x310D, &R3, 1);
		if (rc < 0)
			pr_info("[CAM]%s: i2c_read_b 0x310D fail\n", __func__);

		if ((R3&0x0F) != 0)
			OTP[i] = (short)(R3&0x0F);
		else if ((R2&0x0F) != 0)
			OTP[i] = (short)(R2&0x0F);
		else if ((R2>>4) != 0)
			OTP[i] = (short)(R2>>4);
		else if ((R1&0x0F) != 0)
			OTP[i] = (short)(R1&0x0F);
		else
			OTP[i] = (short)(R1>>4);

	}
	pr_info("[CAM]%s: VenderID=%x,LensID=%x,SensorID=%x%x\n", __func__,
		OTP[0], OTP[1], OTP[2], OTP[3]);
	pr_info("[CAM]%s: ModuleFuseID= %x%x%x%x%x%x\n", __func__,
		OTP[4], OTP[5], OTP[6], OTP[7], OTP[8], OTP[9]);

    cdata->cfg.fuse.fuse_id_word1 = 0;
    cdata->cfg.fuse.fuse_id_word2 = 0;
	cdata->cfg.fuse.fuse_id_word3 = (OTP[0]);
	cdata->cfg.fuse.fuse_id_word4 =
		(OTP[4]<<20) |
		(OTP[5]<<16) |
		(OTP[6]<<12) |
		(OTP[7]<<8) |
		(OTP[8]<<4) |
		(OTP[9]);

	pr_info("[CAM]s5k4e1gx: fuse->fuse_id_word1:%d\n",
		cdata->cfg.fuse.fuse_id_word1);
	pr_info("[CAM]s5k4e1gx: fuse->fuse_id_word2:%d\n",
		cdata->cfg.fuse.fuse_id_word2);
	pr_info("[CAM]s5k4e1gx: fuse->fuse_id_word3:0x%08x\n",
		cdata->cfg.fuse.fuse_id_word3);
	pr_info("[CAM]s5k4e1gx: fuse->fuse_id_word4:0x%08x\n",
		cdata->cfg.fuse.fuse_id_word4);
	return 0;
}

int s5k4e1_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int32_t rc = 0;

	CDBG("%s: %d\n", __func__, __LINE__);
	pr_info("[CAM]Calling s5k4e1_sensor_open_init\n");

	s5k4e1_ctrl = kzalloc(sizeof(struct s5k4e1_ctrl_t), GFP_KERNEL);
	if (!s5k4e1_ctrl) {
		pr_info("[CAM]s5k4e1_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}
	s5k4e1_ctrl->fps_divider = 1 * 0x00000400;
	s5k4e1_ctrl->pict_fps_divider = 1 * 0x00000400;
	s5k4e1_ctrl->set_test = TEST_OFF;
	s5k4e1_ctrl->prev_res = QTR_SIZE;
	s5k4e1_ctrl->pict_res = FULL_SIZE;
	s5k4e1_ctrl->my_reg_gain = 0;
	s5k4e1_ctrl->my_reg_line_count = 0;

	if (data)
		s5k4e1_ctrl->sensordata = data;
/* HTC_START */
	if (data->full_size_preview)
		s5k4e1_ctrl->prev_res = FULL_SIZE;

	/* keep curr_res is different from prev_res to set the first time video_config */
	s5k4e1_ctrl->curr_res = s5k4e1_ctrl->prev_res == FULL_SIZE ?  QTR_SIZE : FULL_SIZE ;

/* HTC_END */
	prev_frame_length_lines =
	((s5k4e1_regs.reg_prev[S5K4E1_REG_PREV_FRAME_LEN_1].wdata << 8) |
		s5k4e1_regs.reg_prev[S5K4E1_REG_PREV_FRAME_LEN_2].wdata);

	prev_line_length_pck =
	(s5k4e1_regs.reg_prev[S5K4E1_REG_PREV_LINE_LEN_1].wdata << 8) |
		s5k4e1_regs.reg_prev[S5K4E1_REG_PREV_LINE_LEN_2].wdata;

	snap_frame_length_lines =
	(s5k4e1_regs.reg_snap[S5K4E1_REG_SNAP_FRAME_LEN_1].wdata << 8) |
		s5k4e1_regs.reg_snap[S5K4E1_REG_SNAP_FRAME_LEN_2].wdata;

	snap_line_length_pck =
	(s5k4e1_regs.reg_snap[S5K4E1_REG_SNAP_LINE_LEN_1].wdata << 8) |
		s5k4e1_regs.reg_snap[S5K4E1_REG_SNAP_LINE_LEN_2].wdata;

	/* enable mclk first */
	msm_camio_clk_rate_set(S5K4E1_MASTER_CLK_RATE);
	mdelay(5);
	rc = s5k4e1_probe_init_sensor(data);
	if (rc < 0)
		goto init_fail;

	/* enable AF actuator */
	if (s5k4e1_ctrl->sensordata->vcm_enable) {
		CDBG("[CAM]enable AF actuator, gpio = %d\n",
			 s5k4e1_ctrl->sensordata->vcm_pwd);
#if 1
		gpio_set_value(s5k4e1_ctrl->sensordata->vcm_pwd, 1);
#else
		rc = gpio_request(s5k4e1_ctrl->sensordata->vcm_pwd,
						"s5k4e1_af");
		if (!rc)
			gpio_direction_output(s5k4e1_ctrl->sensordata->vcm_pwd, 1);
		else {
			pr_err("[CAM]s5k4e1_ctrl gpio request failed!\n");
			goto init_fail;
		}
#endif
		msleep(2);

#ifdef AF_5823_VCM_MODULE /* For 5823 AF VCM module */
	/* set up lens position table */
	s5k4e1_setup_af_tbl();
	s5k4e1_go_to_position(0, 0);
	s5k4e1_ctrl->curr_lens_pos = 0;
	s5k4e1_ctrl->curr_step_pos = 0;
#else /* For 5820 AF VCM module */
		rc = s5k4e1_set_default_focus();
		if (rc < 0) {
			pr_err("[CAM]s5k4e1_set_default_focus: failed!\n");
			gpio_direction_output(s5k4e1_ctrl->sensordata->vcm_pwd,	0);
			gpio_free(s5k4e1_ctrl->sensordata->vcm_pwd);
		}
#endif
	}

	CDBG("init settings\n");
	if (s5k4e1_ctrl->prev_res == QTR_SIZE)
		rc = s5k4e1_sensor_setting(REG_INIT, RES_PREVIEW);
	else {
		if (data->full_size_preview) /* full size preview */
			rc = s5k4e1_sensor_setting(REG_INIT, RES_PREVIEW);
		else
			rc = s5k4e1_sensor_setting(REG_INIT, RES_CAPTURE);
	}
	s5k4e1_ctrl->fps = 30 * Q8;

	if (rc < 0)
		goto init_fail;
	else
		goto init_done;
init_fail:
	pr_info("[CAM]init_fail\n");
	//s5k4e1_probe_init_done(data);
	/* HTC_START : sync from 7x30*/
	if (s5k4e1_ctrl) {
		kfree(s5k4e1_ctrl);
		s5k4e1_ctrl = NULL;
	}
	return rc;
	/* HTC_END */
init_done:
	pr_info("[CAM]init_done\n");
	return rc;
}

static int s5k4e1_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&s5k4e1_wait_queue);
	return 0;
}

static int s5k4e1_af_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&s5k4e1_af_wait_queue);
	return 0;
}

static const struct i2c_device_id s5k4e1_af_i2c_id[] = {
	{"s5k4e1_af", 0},
	{ }
};

static int s5k4e1_af_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int rc = 0;
	CDBG("s5k4e1_af_probe called!\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_info("[CAM]i2c_check_functionality failed\n");
		goto probe_failure;
	}

	s5k4e1_af_sensorw = kzalloc(sizeof(struct s5k4e1_work_t), GFP_KERNEL);
	if (!s5k4e1_af_sensorw) {
		pr_info("[CAM]kzalloc failed.\n");
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, s5k4e1_af_sensorw);
	s5k4e1_af_init_client(client);
	s5k4e1_af_client = client;

	msleep(50);

	pr_info("[CAM]s5k4e1_af_probe successed! rc = %d\n", rc);
	return 0;

probe_failure:
	pr_info("[CAM]s5k4e1_af_probe failed! rc = %d\n", rc);
	return rc;
}

static const struct i2c_device_id s5k4e1_i2c_id[] = {
	{"s5k4e1", 0},
	{ }
};

static int s5k4e1_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int rc = 0;
	pr_info("[CAM]s5k4e1_probe called!\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_info("[CAM]i2c_check_functionality failed\n");
		goto probe_failure;
	}

	s5k4e1_sensorw = kzalloc(sizeof(struct s5k4e1_work_t), GFP_KERNEL);
	if (!s5k4e1_sensorw) {
		pr_info("[CAM]kzalloc failed.\n");
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, s5k4e1_sensorw);
	s5k4e1_init_client(client);
	s5k4e1_client = client;

	msleep(50);

	pr_info("[CAM]s5k4e1_probe successed! rc = %d\n", rc);
	return 0;

probe_failure:
	pr_info("[CAM]s5k4e1_probe failed! rc = %d\n", rc);
	return rc;
}

static int __devexit s5k4e1_remove(struct i2c_client *client)
{
	struct s5k4e1_work_t *sensorw = i2c_get_clientdata(client);
	free_irq(client->irq, sensorw);
	s5k4e1_client = NULL;
	kfree(sensorw);
	return 0;
}

static int __devexit s5k4e1_af_remove(struct i2c_client *client)
{
	struct s5k4e1_work_t *s5k4e1_af = i2c_get_clientdata(client);
	free_irq(client->irq, s5k4e1_af);
	s5k4e1_af_client = NULL;
	kfree(s5k4e1_af);
	return 0;
}

static struct i2c_driver s5k4e1_i2c_driver = {
	.id_table = s5k4e1_i2c_id,
	.probe  = s5k4e1_i2c_probe,
	.remove = __exit_p(s5k4e1_i2c_remove),
	.driver = {
		.name = "s5k4e1",
	},
};

static struct i2c_driver s5k4e1_af_i2c_driver = {
	.id_table = s5k4e1_af_i2c_id,
	.probe  = s5k4e1_af_i2c_probe,
	.remove = __exit_p(s5k4e1_af_i2c_remove),
	.driver = {
		.name = "s5k4e1_af",
	},
};

int s5k4e1_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cdata;
	long   rc = 0;
	if (copy_from_user(&cdata,
				(void *)argp,
				sizeof(struct sensor_cfg_data)))
		return -EFAULT;
	mutex_lock(&s5k4e1_mut);
	CDBG("s5k4e1_sensor_config: cfgtype = %d\n",
			cdata.cfgtype);
	switch (cdata.cfgtype) {
	case CFG_GET_PICT_FPS:
		s5k4e1_get_pict_fps(
			cdata.cfg.gfps.prevfps,
			&(cdata.cfg.gfps.pictfps));

		if (copy_to_user((void *)argp,
			&cdata,
			sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	case CFG_GET_PREV_L_PF:
		cdata.cfg.prevl_pf =
			s5k4e1_get_prev_lines_pf();

		if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	case CFG_GET_PREV_P_PL:
		cdata.cfg.prevp_pl =
			s5k4e1_get_prev_pixels_pl();

		if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	case CFG_GET_PICT_L_PF:
		cdata.cfg.pictl_pf =
			s5k4e1_get_pict_lines_pf();

		if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	case CFG_GET_PICT_P_PL:
		cdata.cfg.pictp_pl =
			s5k4e1_get_pict_pixels_pl();
		if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	case CFG_GET_PICT_MAX_EXP_LC:
		cdata.cfg.pict_max_exp_lc =
			s5k4e1_get_pict_max_exp_lc();

		if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	case CFG_SET_FPS:
	case CFG_SET_PICT_FPS:
		rc = s5k4e1_set_fps(&(cdata.cfg.fps));
		break;
	case CFG_SET_EXP_GAIN:
		rc = s5k4e1_write_exp_gain(cdata.cfg.exp_gain.gain,
				cdata.cfg.exp_gain.line);
		break;
	case CFG_SET_PICT_EXP_GAIN:
		rc = s5k4e1_set_pict_exp_gain(cdata.cfg.exp_gain.gain,
				cdata.cfg.exp_gain.line);
		break;
	case CFG_SET_MODE:
		rc = s5k4e1_set_sensor_mode(cdata.mode, cdata.rs);
		break;
	case CFG_PWR_DOWN:
		rc = s5k4e1_power_down();
		break;
	case CFG_MOVE_FOCUS:
		rc = s5k4e1_move_focus(cdata.cfg.focus.dir,
				cdata.cfg.focus.steps);
		break;
	case CFG_SET_DEFAULT_FOCUS:
		rc = s5k4e1_set_default_focus();
		break;
	case CFG_GET_AF_MAX_STEPS:
		cdata.max_steps = S5K4E1_TOTAL_STEPS_NEAR_TO_FAR;
		if (copy_to_user((void *)argp,
					&cdata,
				sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	case CFG_I2C_IOCTL_R_OTP:
		pr_info("[CAM]Line:%d CFG_I2C_IOCTL_R_OTP \n", __LINE__);
		rc = s5k4e1_i2c_read_fuseid(&cdata);
		if (copy_to_user(argp, &cdata, sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	case CFG_SET_EFFECT:
	default:
		rc = -EFAULT;
		break;
	}
	mutex_unlock(&s5k4e1_mut);

	return rc;
}

static int s5k4e1_sensor_release(void)
{
	int rc = -EBADF;

	mutex_lock(&s5k4e1_mut);
	s5k4e1_power_down();
	msleep(2);
#if 1
	/* HTC_START */
	gpio_set_value_cansleep(s5k4e1_ctrl->sensordata->sensor_reset, 0);
	usleep_range(5000, 5100);
	if (s5k4e1_ctrl->sensordata->vcm_enable)
		gpio_set_value_cansleep(s5k4e1_ctrl->sensordata->vcm_pwd, 0);

	msleep(1);//shuji 0119
    /* HTC_END */
#else
	gpio_set_value_cansleep(s5k4e1_ctrl->sensordata->sensor_reset, 0);
	usleep_range(5000, 5100);
//	gpio_free(s5k4e1_ctrl->sensordata->sensor_reset);
	if (s5k4e1_ctrl->sensordata->vcm_enable) {
		gpio_set_value_cansleep(s5k4e1_ctrl->sensordata->vcm_pwd, 0);
		gpio_free(s5k4e1_ctrl->sensordata->vcm_pwd);
	}
#endif
	kfree(s5k4e1_ctrl);
	s5k4e1_ctrl = NULL;
	CSI_CONFIG = 0;
	pr_info("[CAM]s5k4e1_release completed\n");
	mutex_unlock(&s5k4e1_mut);

	return rc;
}

/* HTC_START */
static int sensor_probe_node = 0;
static int fps_mode_sel;

static const char *S5K4E1GXVendor = "samsung";
static const char *S5K4E1GXNAME = "s5k4e1gx";
static const char *S5K4E1GXSize = "5M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", S5K4E1GXVendor, S5K4E1GXNAME, S5K4E1GXSize);
	ret = strlen(buf) + 1;

	return ret;
}

DEFINE_MUTEX(fps_mode_lock);

static ssize_t sensor_read_fps_mode(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t length;
	mutex_lock(&fps_mode_lock);
	length = sprintf(buf, "%d\n", fps_mode_sel);
	mutex_unlock(&fps_mode_lock);
	return length;
}

static ssize_t sensor_set_fps_mode(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	uint32_t tmp = 0;
	mutex_lock(&fps_mode_lock);
	tmp = buf[0] - 0x30;
	fps_mode_sel = tmp;
	mutex_unlock(&fps_mode_lock);
	return count;
}

static ssize_t sensor_read_node(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t length;
	length = sprintf(buf, "%d\n", sensor_probe_node);
	return length;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);
static DEVICE_ATTR(node, 0444, sensor_read_node, NULL);
static DEVICE_ATTR(fps_mode, 0777,
	sensor_read_fps_mode, sensor_set_fps_mode);

static struct kobject *android_s5k4e1gx = NULL;

static int s5k4e1gx_sysfs_init(void)
{
	int ret ;
	CDBG("s5k4e1gx_sysfs_init : kobject_create_and_add\n");
	android_s5k4e1gx = kobject_create_and_add("android_camera", NULL);
	if (android_s5k4e1gx == NULL) {
		pr_info("[CAM]s5k4e1gx_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	printk(KERN_INFO "[CAM]s5k4e1gx_sysfs_init : sysfs_create_file\n");
	ret = sysfs_create_file(android_s5k4e1gx, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("[CAM]s5k4e1gx_sysfs_init : sysfs_create_file " \
		"failed\n");
		kobject_del(android_s5k4e1gx);
	}
	ret = sysfs_create_file(android_s5k4e1gx, &dev_attr_fps_mode.attr);
	if (ret) {
		pr_info("[CAM]s5k4e1gx_sysfs_init : sysfs_create_file " \
		"failed\n");
		ret = -EFAULT;
		return ret ;
	}
        ret = sysfs_create_file(android_s5k4e1gx, &dev_attr_node.attr);
	if (ret) {
		pr_info("[CAM]s5k4e1gx_sysfs_init: dev_attr_node failed\n");
		ret = -EFAULT;
		return ret;
	}

	return 0 ;
}
/* HTC_END */

static int s5k4e1_sensor_probe(const struct msm_camera_sensor_info *info,
		struct msm_sensor_ctrl *s)
{
	int rc = 0;

	rc = i2c_add_driver(&s5k4e1_i2c_driver);
	if (rc < 0 || s5k4e1_client == NULL) {
		rc = -ENOTSUPP;
		pr_info("[CAM]I2C add driver failed");
		goto probe_fail_1;
	}

	if(info->vcm_enable){
		rc = i2c_add_driver(&s5k4e1_af_i2c_driver);
		if (rc < 0 || s5k4e1_af_client == NULL) {
			rc = -ENOTSUPP;
			pr_info("[CAM]I2C add driver failed");
			goto probe_fail_2;
		}
	}

	msm_camio_clk_rate_set(S5K4E1_MASTER_CLK_RATE);

	msleep(2);

	rc = s5k4e1_probe_init_sensor(info);
	if (rc < 0)
		goto probe_fail_3;

	s->s_init = s5k4e1_sensor_open_init;
	s->s_release = s5k4e1_sensor_release;
	s->s_config  = s5k4e1_sensor_config;
    msleep(2);
	gpio_set_value(info->sensor_reset, 0);
	msleep(1);//shuji 0119
/* HTC START */
#if 0
	s->s_mount_angle = info->sensor_platform_info->mount_angle;
	gpio_set_value_cansleep(info->sensor_reset, 0);
	s5k4e1_probe_init_done(info);
#endif
/* HTC_END */

#if 0 /* Mark it due to this action is not done at probe stage */
	/* Keep vcm_pwd to OUT Low */
	if (info->vcm_enable) {
		rc = gpio_request(info->vcm_pwd, "s5k4e1_af");
		if (!rc) {
			gpio_direction_output(info->vcm_pwd, 0);
			msleep(20);
			gpio_free(info->vcm_pwd);
		} else
			return rc;
	}
#endif
	/* HTC_START */
	s5k4e1gx_sysfs_init();
	pr_info("[CAM]s5k4e1_sensor_probe: SENSOR PROBE OK!\n");
	/* HTC_END */
	return rc;

probe_fail_3:
      if(info->vcm_enable)
	i2c_del_driver(&s5k4e1_af_i2c_driver);
probe_fail_2:
	i2c_del_driver(&s5k4e1_i2c_driver);
probe_fail_1:
	pr_info("[CAM]s5k4e1_sensor_probe: SENSOR PROBE FAILS!\n");
	return rc;
}

static int __devinit s5k4e1_probe(struct platform_device *pdev)
{
	printk("[CAM]s5k4e1_probe\n");
	return msm_camera_drv_start(pdev, s5k4e1_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = s5k4e1_probe,
	.driver = {
		.name = "msm_camera_s5k4e1",
		.owner = THIS_MODULE,
	},
};

static int __init s5k4e1_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(s5k4e1_init);
MODULE_DESCRIPTION("Samsung 5 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
