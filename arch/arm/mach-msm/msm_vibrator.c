/* include/asm/mach-msm/htc_pwrsink.h
 *
 * Copyright (C) 2008 HTC Corporation.
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2011 Code Aurora Forum. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *when           who    why   what
 */

#define DEBUG  0

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>
#include "pmic.h"
#include "timed_output.h"

#include <mach/msm_rpcrouter.h>

#ifdef CONFIG_MSM_RPC_VIBRATOR
#define PM_LIBPROG	0x30000061
#ifdef  CONFIG_RPC_VER_60001
#define PM_LIBVERS	0x00060001
#else
#define PM_LIBVERS	0x00030001
#endif
#define HTC_PROCEDURE_SET_VIB_ON_OFF	22
#define VIB_OFF_DELAY  50  //ms

#define PMIC_VIBRATOR_LEVEL	(3000)

#if DEBUG
#define debug_print(x...) do {pr_info(x); } while (0)
#else
#define debug_print(x...) do {} while (0)
#endif

static struct work_struct work_vibrator_on;
static struct work_struct work_vibrator_off;
static struct hrtimer vibe_timer;

static volatile int g_vibrator_status=0;  //represent the vibrator's real on off status; 1:on  0:off
#if DEBUG
struct timespec volatile g_ts_start;
struct timespec volatile g_ts_end;
#endif
static int vibrator_on_delay;

static void set_pmic_vibrator(int on)
{
	static struct msm_rpc_endpoint *vib_endpoint;
	struct set_vib_on_off_req
	{
		struct rpc_request_hdr hdr;
		uint32_t data;
	}
	req;

	if (!vib_endpoint)
	{
		vib_endpoint = msm_rpc_connect(PM_LIBPROG, PM_LIBVERS, 0);
		if (IS_ERR(vib_endpoint))
		{
			printk(KERN_ERR "init vib rpc failed!\n");
			vib_endpoint = 0;
			return;
		}
	}

	if (on)
		req.data = cpu_to_be32(PMIC_VIBRATOR_LEVEL);
	else
		req.data = cpu_to_be32(0);

	msm_rpc_call(vib_endpoint, HTC_PROCEDURE_SET_VIB_ON_OFF, &req,
			sizeof(req), 5 * HZ);

	if(on)
	{
		g_vibrator_status=1;
#if DEBUG
		g_ts_start = current_kernel_time();
#endif
	}
	else
	{
		if(g_vibrator_status==1)
		{
			g_vibrator_status=0;
#if DEBUG
			g_ts_end = current_kernel_time();
			pr_info("vibrator vibrated %ld ms.\n",
					(g_ts_end.tv_sec-g_ts_start.tv_sec)*1000+g_ts_end.tv_nsec/1000000-g_ts_start.tv_nsec/1000000 );
#endif
		}
	}
}

static void pmic_vibrator_on(struct work_struct *work)
{
	if( g_vibrator_status==0)  //if vibrator is on now
	{
		debug_print("Q:pmic_vibrator_on,start");
		set_pmic_vibrator(1);
		debug_print("Q:pmic_vibrator_on,done\n");
	}
	else
	{
		debug_print("Q:pmic_vibrator_on, already on, do nothing.\n");
	}
		vibrator_on_delay = (vibrator_on_delay > 15000 ? 15000 : vibrator_on_delay);  //moved from enable fun & modified chenchongbao.20111218
		hrtimer_start(&vibe_timer,
				ktime_set(vibrator_on_delay / 1000, (vibrator_on_delay % 1000 ) * 1000000),
				HRTIMER_MODE_REL);
}

static void pmic_vibrator_off(struct work_struct *work)
{
	if( g_vibrator_status==1)  //if vibrator is on now
	{
		debug_print("Q:pmic_vibrator_off,start");
		set_pmic_vibrator(0);
		debug_print("Q:pmic_vibrator_off,done\n");
	}
	else
	{
		debug_print("Q:pmic_vibrator_off, already off, do nothing.\n");
	}
}

static int timed_vibrator_on(struct timed_output_dev *sdev)
{
	if(!schedule_work(&work_vibrator_on))
	{
		pr_info("vibrator schedule on work failed !\n");
		return 0;
	}
	return 1;
}

static int timed_vibrator_off(struct timed_output_dev *sdev)
{
	if(!schedule_work(&work_vibrator_off))
	{
		pr_info("vibrator schedule off work failed !\n");
		return 0;
	}
	return 1;
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	hrtimer_cancel(&vibe_timer);
	cancel_work_sync(&work_vibrator_on);
	cancel_work_sync(&work_vibrator_off);

	pr_info("vibrator_enable,%d ms,vibrator:%s now.\n",value,g_vibrator_status?"on":"off");

	if (value == 0)
	{
		if(!timed_vibrator_off(dev))  //if queue failed, delay 10ms try again by timer
		{
			pr_info("vibrator_enable, queue failed!\n");
			value=10;
			hrtimer_start(&vibe_timer,
					ktime_set(value / 1000, (value % 1000) * 1000000),
					HRTIMER_MODE_REL);
			value=0;
		}
	}
	else
	{
		vibrator_on_delay = value;
		timed_vibrator_on(dev);
	}
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	if (hrtimer_active(&vibe_timer))
	{
		ktime_t r = hrtimer_get_remaining(&vibe_timer);
		struct timeval t = ktime_to_timeval(r);
		return t.tv_sec * 1000 + t.tv_usec / 1000;
	}
	else
		return 0;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	int value=0;
	debug_print("timer:vibrator timeout!\n");
	if(!timed_vibrator_off(NULL))
	{
		pr_info("timer:vibrator timeout queue failed!\n");
		value=500;
		hrtimer_start(&vibe_timer,
				ktime_set(value / 1000, (value % 1000) * 1000000),
				HRTIMER_MODE_REL);
		value=0;
	}
	return HRTIMER_NORESTART;
}

static struct timed_output_dev pmic_vibrator =
{
	.name = "vibrator",
	.get_time = vibrator_get_time,
	.enable = vibrator_enable,
};

void __init msm_init_pmic_vibrator(void)
{
	pr_info("msm_init_pmic_vibrator\n");
	INIT_WORK(&work_vibrator_on, pmic_vibrator_on);
	INIT_WORK(&work_vibrator_off, pmic_vibrator_off);

	hrtimer_init(&vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibe_timer.function = vibrator_timer_func;

	timed_output_dev_register(&pmic_vibrator);
}

MODULE_DESCRIPTION("timed output pmic vibrator device");
MODULE_LICENSE("GPL");

#else
#define PM_LIBPROG      0x30000061
#if (CONFIG_MSM_AMSS_VERSION == 6220) || (CONFIG_MSM_AMSS_VERSION == 6225)
#define PM_LIBVERS      0xfb837d0b
#else
#define PM_LIBVERS      0x10001
#endif

#define HTC_PROCEDURE_SET_VIB_ON_OFF	21
#define PMIC_VIBRATOR_LEVEL	(3000)

static struct work_struct work_vibrator_on;
static struct work_struct work_vibrator_off;
static struct hrtimer vibe_timer;

#ifdef CONFIG_PM8XXX_RPC_VIBRATOR
static void set_pmic_vibrator(int on)
{
	int rc;

	rc = pmic_vib_mot_set_mode(PM_VIB_MOT_MODE__MANUAL);
	if (rc) {
		pr_err("%s: Vibrator set mode failed", __func__);
		return;
	}

	if (on)
		rc = pmic_vib_mot_set_volt(PMIC_VIBRATOR_LEVEL);
	else
		rc = pmic_vib_mot_set_volt(0);

	if (rc)
		pr_err("%s: Vibrator set voltage level failed", __func__);
}
#else
static void set_pmic_vibrator(int on)
{
	static struct msm_rpc_endpoint *vib_endpoint;
	struct set_vib_on_off_req {
		struct rpc_request_hdr hdr;
		uint32_t data;
	} req;

	if (!vib_endpoint) {
		vib_endpoint = msm_rpc_connect(PM_LIBPROG, PM_LIBVERS, 0);
		if (IS_ERR(vib_endpoint)) {
			printk(KERN_ERR "init vib rpc failed!\n");
			vib_endpoint = 0;
			return;
		}
	}


	if (on)
		req.data = cpu_to_be32(PMIC_VIBRATOR_LEVEL);
	else
		req.data = cpu_to_be32(0);

	msm_rpc_call(vib_endpoint, HTC_PROCEDURE_SET_VIB_ON_OFF, &req,
		sizeof(req), 5 * HZ);
}
#endif

static void pmic_vibrator_on(struct work_struct *work)
{
	set_pmic_vibrator(1);
}

static void pmic_vibrator_off(struct work_struct *work)
{
	set_pmic_vibrator(0);
}

static void timed_vibrator_on(struct timed_output_dev *sdev)
{
	schedule_work(&work_vibrator_on);
}

static void timed_vibrator_off(struct timed_output_dev *sdev)
{
	schedule_work(&work_vibrator_off);
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	hrtimer_cancel(&vibe_timer);

	if (value == 0)
		timed_vibrator_off(dev);
	else {
		value = (value > 15000 ? 15000 : value);

		timed_vibrator_on(dev);

		hrtimer_start(&vibe_timer,
			      ktime_set(value / 1000, (value % 1000) * 1000000),
			      HRTIMER_MODE_REL);
	}
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	if (hrtimer_active(&vibe_timer)) {
		ktime_t r = hrtimer_get_remaining(&vibe_timer);
		struct timeval t = ktime_to_timeval(r);
		return t.tv_sec * 1000 + t.tv_usec / 1000;
	}
	return 0;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	timed_vibrator_off(NULL);
	return HRTIMER_NORESTART;
}

static struct timed_output_dev pmic_vibrator = {
	.name = "vibrator",
	.get_time = vibrator_get_time,
	.enable = vibrator_enable,
};

void __init msm_init_pmic_vibrator(void)
{
	INIT_WORK(&work_vibrator_on, pmic_vibrator_on);
	INIT_WORK(&work_vibrator_off, pmic_vibrator_off);

	hrtimer_init(&vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibe_timer.function = vibrator_timer_func;

	timed_output_dev_register(&pmic_vibrator);
}

MODULE_DESCRIPTION("timed output pmic vibrator device");
MODULE_LICENSE("GPL");
#endif

