/* arch/arm/mach-msm/qdsp5/audmgr.c
 *
 * interface to "audmgr" service on the baseband cpu
 *
 * Copyright (C) 2008 Google, Inc.
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
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
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/io.h>

#include <asm/atomic.h>
#include <mach/msm_rpcrouter.h>

#include "audmgr.h"
#include <mach/debug_mm.h>
#include <mach/msm_iomap.h>

#define STATE_CLOSED    0
#define STATE_DISABLED  1
#define STATE_ENABLING  2
#define STATE_ENABLED   3
#define STATE_DISABLING 4
#define STATE_ERROR	5

/* store information used across complete audmgr sessions */
struct audmgr_global {
	struct mutex *lock;
	struct msm_rpc_endpoint *ept;
	struct task_struct *task;
	uint32_t rpc_version;
};
static DEFINE_MUTEX(audmgr_lock);

static struct audmgr_global the_audmgr_state = {
	.lock = &audmgr_lock,
};

static void rpc_ack(struct msm_rpc_endpoint *ept, uint32_t xid)
{
	uint32_t rep[6];

	rep[0] = cpu_to_be32(xid);
	rep[1] = cpu_to_be32(1);
	rep[2] = cpu_to_be32(RPCMSG_REPLYSTAT_ACCEPTED);
	rep[3] = cpu_to_be32(RPC_ACCEPTSTAT_SUCCESS);
	rep[4] = 0;
	rep[5] = 0;

	msm_rpc_write(ept, rep, sizeof(rep));
}

static void process_audmgr_callback(struct audmgr_global *amg,
				   struct rpc_audmgr_cb_func_ptr *args,
				   int len)
{
	struct audmgr *am;

	/* Allow only if complete arguments recevied */
	if (len < (sizeof(struct rpc_audmgr_cb_func_ptr)))
		return;

	/* Allow only if valid argument */
	if (be32_to_cpu(args->set_to_one) != 1)
		return;

	am = (struct audmgr *) be32_to_cpu(args->client_data);

	if (!am)
		return;

	switch (be32_to_cpu(args->status)) {
	case RPC_AUDMGR_STATUS_READY:
		am->handle = be32_to_cpu(args->u.handle);
		MM_AUD_INFO("audmgr: rpc READY handle=0x%08x\n", am->handle);
		break;
	case RPC_AUDMGR_STATUS_CODEC_CONFIG: {
		uint32_t volume;
		volume = be32_to_cpu(args->u.volume);
		MM_AUD_INFO("audmgr: rpc CODEC_CONFIG volume=0x%08x\n", volume);
		am->state = STATE_ENABLED;
		wake_up(&am->wait);
		break;
	}
	case RPC_AUDMGR_STATUS_PENDING:
		MM_AUD_ERR("audmgr: PENDING?\n");
		break;
	case RPC_AUDMGR_STATUS_SUSPEND:
		MM_AUD_ERR("audmgr: SUSPEND?\n");
		break;
	case RPC_AUDMGR_STATUS_FAILURE:
		MM_AUD_ERR("audmgr: FAILURE\n");
		break;
	case RPC_AUDMGR_STATUS_VOLUME_CHANGE:
		MM_AUD_ERR("audmgr: VOLUME_CHANGE?\n");
		break;
	case RPC_AUDMGR_STATUS_DISABLED:
		MM_AUD_INFO("audmgr: DISABLED\n");
		am->state = STATE_DISABLED;
		wake_up(&am->wait);
		break;
	case RPC_AUDMGR_STATUS_ERROR:
		MM_AUD_ERR("audmgr: ERROR?\n");
		am->state = STATE_ERROR;
		wake_up(&am->wait);
		break;
	default:
		break;
	}
}

static void process_rpc_request(uint32_t proc, uint32_t xid,
				void *data, int len, void *private)
{
	struct audmgr_global *amg = private;
	/*
	uint32_t *x = data;

	if (0) {
		int n = len / 4;
		MM_AUD_INFO("rpc_call proc %d:", proc);
		while (n--)
			MM_AUD_INFO(" %08x", be32_to_cpu(*x++));
		printk(KERN_INFO "\n");
	}
	*/
	if (proc == AUDMGR_CB_FUNC_PTR)
		process_audmgr_callback(amg, data, len);
	else
		MM_AUD_ERR("audmgr: unknown rpc proc %d\n", proc);
	rpc_ack(amg->ept, xid);
}

#define RPC_TYPE_REQUEST 0
#define RPC_TYPE_REPLY 1

#define RPC_VERSION 2

#define RPC_COMMON_HDR_SZ  (sizeof(uint32_t) * 2)
#define RPC_REQUEST_HDR_SZ (sizeof(struct rpc_request_hdr))
#define RPC_REPLY_HDR_SZ   (sizeof(uint32_t) * 3)
#define RPC_REPLY_SZ       (sizeof(uint32_t) * 6)

static void dprint_int_ctrl_regs(void)
{
	int n;

	printk(KERN_INFO "DPRINT_INT_CTRL_REGS:\n\n");
	printk(KERN_INFO "    VIC_INT_SELECT0(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x00), (unsigned int)readl(MSM_VIC_BASE + 0x00));
	printk(KERN_INFO "    VIC_INT_SELECT1(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x04), (unsigned int)readl(MSM_VIC_BASE + 0x04));
	printk(KERN_INFO "        VIC_INT_EN0(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x10), (unsigned int)readl(MSM_VIC_BASE + 0x10));
	printk(KERN_INFO "        VIC_INT_EN1(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x14), (unsigned int)readl(MSM_VIC_BASE + 0x14));
	printk(KERN_INFO "   VIC_INT_ENCLEAR0(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x20), (unsigned int)readl(MSM_VIC_BASE + 0x20));
	printk(KERN_INFO "   VIC_INT_ENCLEAR1(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x24), (unsigned int)readl(MSM_VIC_BASE + 0x24));
	printk(KERN_INFO "     VIC_INT_ENSET0(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x30), (unsigned int)readl(MSM_VIC_BASE + 0x30));
	printk(KERN_INFO "     VIC_INT_ENSET1(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x34), (unsigned int)readl(MSM_VIC_BASE + 0x34));
	printk(KERN_INFO "      VIC_INT_TYPE0(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x40), (unsigned int)readl(MSM_VIC_BASE + 0x40));
	printk(KERN_INFO "      VIC_INT_TYPE1(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x44), (unsigned int)readl(MSM_VIC_BASE + 0x44));
	printk(KERN_INFO "  VIC_INT_POLARITY0(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x50), (unsigned int)readl(MSM_VIC_BASE + 0x50));
	printk(KERN_INFO "  VIC_INT_POLARITY1(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x54), (unsigned int)readl(MSM_VIC_BASE + 0x54));
	printk(KERN_INFO "    VIC_NO_PEND_VAL(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x60), (unsigned int)readl(MSM_VIC_BASE + 0x60));
	printk(KERN_INFO "   VIC_INT_MASTEREN(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x64), (unsigned int)readl(MSM_VIC_BASE + 0x64));
	printk(KERN_INFO "     VIC_PROTECTION(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x6C), (unsigned int)readl(MSM_VIC_BASE + 0x6C));
	printk(KERN_INFO "         VIC_CONFIG(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x68), (unsigned int)readl(MSM_VIC_BASE + 0x68));
	printk(KERN_INFO "    VIC_IRQ_STATUS0(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x80), (unsigned int)readl(MSM_VIC_BASE + 0x80));
	printk(KERN_INFO "    VIC_IRQ_STATUS1(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x84), (unsigned int)readl(MSM_VIC_BASE + 0x84));
	printk(KERN_INFO "    VIC_FIQ_STATUS0(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x90), (unsigned int)readl(MSM_VIC_BASE + 0x90));
	printk(KERN_INFO "    VIC_FIQ_STATUS1(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0x94), (unsigned int)readl(MSM_VIC_BASE + 0x94));
	printk(KERN_INFO "    VIC_RAW_STATUS0(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0xA0), (unsigned int)readl(MSM_VIC_BASE + 0xA0));
	printk(KERN_INFO "    VIC_RAW_STATUS1(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0xA4), (unsigned int)readl(MSM_VIC_BASE + 0xA4));
	printk(KERN_INFO "     VIC_INT_CLEAR0(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0xB0), (unsigned int)readl(MSM_VIC_BASE + 0xB0));
	printk(KERN_INFO "     VIC_INT_CLEAR1(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0xB4), (unsigned int)readl(MSM_VIC_BASE + 0xB4));
	printk(KERN_INFO "       VIC_SOFTINT0(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0xC0), (unsigned int)readl(MSM_VIC_BASE + 0xC0));
	printk(KERN_INFO "       VIC_SOFTINT1(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0xC4), (unsigned int)readl(MSM_VIC_BASE + 0xC4));
	printk(KERN_INFO "     VIC_IRQ_VEC_RD(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0xD0), (unsigned int)readl(MSM_VIC_BASE + 0xD0));
	printk(KERN_INFO "VIC_IRQ_VEC_PEND_RD(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0xD4), (unsigned int)readl(MSM_VIC_BASE + 0xD4));
	printk(KERN_INFO "     VIC_IRQ_VEC_WR(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0xD8), (unsigned int)readl(MSM_VIC_BASE + 0xD8));
	printk(KERN_INFO " VIC_IRQ_IN_SERVICE(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0xE0), (unsigned int)readl(MSM_VIC_BASE + 0xE0));
	printk(KERN_INFO "   VIC_IRQ_IN_STACK(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0xE4), (unsigned int)readl(MSM_VIC_BASE + 0xE4));
	printk(KERN_INFO "   VIC_TEST_BUS_SEL(0x%X) : 0x%X\n", (unsigned int)(MSM_VIC_BASE + 0xE8), (unsigned int)readl(MSM_VIC_BASE + 0xE8));

	for (n = 0; n < 64; n++)
		printk(KERN_INFO " VIC_TEST_BUS_SEL_%d(0x%X) : 0x%X\n", n, (unsigned int)(MSM_VIC_BASE + 0x200 + 4*n), (unsigned int)readl(MSM_VIC_BASE + 0x200 + 4*n));
	for (n = 0; n < 64; n++)
		printk(KERN_INFO "  VIC_VECTADDR_%d(0x%X) : 0x%X\n", n, (unsigned int)(MSM_VIC_BASE + 0x400 + 4*n), (unsigned int)readl(MSM_VIC_BASE + 0x400 + 4*n));
}

static int audmgr_rpc_thread(void *data)
{
	struct audmgr_global *amg = data;
	struct rpc_request_hdr *hdr = NULL;
	uint32_t type;
	int len;

	MM_AUD_INFO("audmgr_rpc_thread() start\n");

	while (!kthread_should_stop()) {
		if (hdr) {
			kfree(hdr);
			hdr = NULL;
		}
		len = msm_rpc_read(amg->ept, (void **) &hdr, -1, -1);
		if (len < 0) {
			MM_AUD_ERR("audmgr: rpc read failed (%d)\n", len);
			break;
		}
		if (len < RPC_COMMON_HDR_SZ)
			continue;

		type = be32_to_cpu(hdr->type);
		if (type == RPC_TYPE_REPLY) {
			struct rpc_reply_hdr *rep = (void *) hdr;
			uint32_t status;
			if (len < RPC_REPLY_HDR_SZ)
				continue;
			status = be32_to_cpu(rep->reply_stat);
			if (status == RPCMSG_REPLYSTAT_ACCEPTED) {
				status = be32_to_cpu(rep->data.acc_hdr.accept_stat);
				MM_AUD_INFO("audmgr: "
					"rpc_reply status %d\n", status);
			} else {
				MM_AUD_INFO("audmgr: rpc_reply denied!\n");
			}
			/* process reply */
			continue;
		}

		if (len < RPC_REQUEST_HDR_SZ)
			continue;

		process_rpc_request(be32_to_cpu(hdr->procedure),
				    be32_to_cpu(hdr->xid),
				    (void *) (hdr + 1),
				    len - sizeof(*hdr),
				    data);
	}
	MM_AUD_INFO("audmgr_rpc_thread() exit\n");
	if (hdr) {
		kfree(hdr);
		hdr = NULL;
	}
	amg->task = NULL;
	return 0;
}

struct audmgr_enable_msg {
	struct rpc_request_hdr hdr;
	struct rpc_audmgr_enable_client_args args;
};

struct audmgr_disable_msg {
	struct rpc_request_hdr hdr;
	uint32_t handle;
};

int audmgr_open(struct audmgr *am)
{
	struct audmgr_global *amg = &the_audmgr_state;
	int rc;

	if (am->state != STATE_CLOSED)
		return 0;

	mutex_lock(amg->lock);

	/* connect to audmgr end point and polling thread only once */
	if (amg->ept == NULL) {
		amg->ept = msm_rpc_connect_compatible(AUDMGR_PROG,
				AUDMGR_VERS_COMP_VER3,
				MSM_RPC_UNINTERRUPTIBLE);
		if (IS_ERR(amg->ept)) {
			MM_AUD_ERR("audmgr:connect failed with current VERS \
				= %x, trying again with another API\n",
				AUDMGR_VERS_COMP_VER3);
			amg->ept = msm_rpc_connect_compatible(AUDMGR_PROG,
					AUDMGR_VERS_COMP_VER2,
					MSM_RPC_UNINTERRUPTIBLE);
			if (IS_ERR(amg->ept)) {
				MM_AUD_ERR("audmgr:connect failed with current \
					VERS = %x, trying again with another \
					API\n",	AUDMGR_VERS_COMP_VER2);
				amg->ept = msm_rpc_connect_compatible(
						AUDMGR_PROG,
						AUDMGR_VERS_COMP,
						MSM_RPC_UNINTERRUPTIBLE);
				if (IS_ERR(amg->ept)) {
					MM_AUD_ERR("audmgr:connect failed with \
					current VERS=%x, trying again with \
					another API\n", AUDMGR_VERS_COMP);
					amg->ept = msm_rpc_connect(AUDMGR_PROG,
						AUDMGR_VERS,
						MSM_RPC_UNINTERRUPTIBLE);
					amg->rpc_version = AUDMGR_VERS;
				} else
					amg->rpc_version = AUDMGR_VERS_COMP;
			} else
				amg->rpc_version = AUDMGR_VERS_COMP_VER2;
		} else
			amg->rpc_version = AUDMGR_VERS_COMP_VER3;

		if (IS_ERR(amg->ept)) {
			rc = PTR_ERR(amg->ept);
			amg->ept = NULL;
			MM_AUD_ERR("audmgr:failed to connect to audmgr svc\n");
			goto done;
		}

		amg->task = kthread_run(audmgr_rpc_thread, amg, "audmgr_rpc");
		if (IS_ERR(amg->task)) {
			rc = PTR_ERR(amg->task);
			amg->task = NULL;
			msm_rpc_close(amg->ept);
			amg->ept = NULL;
			goto done;
		}
	}

	/* Initialize session parameters */
	init_waitqueue_head(&am->wait);
	am->state = STATE_DISABLED;
	rc = 0;
done:
	mutex_unlock(amg->lock);
	return rc;
}
EXPORT_SYMBOL(audmgr_open);

int audmgr_close(struct audmgr *am)
{
	return -EBUSY;
}
EXPORT_SYMBOL(audmgr_close);

int audmgr_enable(struct audmgr *am, struct audmgr_config *cfg)
{
	struct audmgr_global *amg = &the_audmgr_state;
	struct audmgr_enable_msg msg;
	int rc;

	if (am->state == STATE_ENABLED)
		return 0;

	if (am->state == STATE_DISABLING)
		MM_AUD_ERR("audmgr: state is DISABLING in enable?\n");

	am->state = STATE_ENABLING;

	MM_AUD_INFO("audmgr: session 0x%08x\n", (int) am);
	msg.args.set_to_one = cpu_to_be32(1);
	msg.args.tx_sample_rate = cpu_to_be32(cfg->tx_rate);
	msg.args.rx_sample_rate = cpu_to_be32(cfg->rx_rate);
	msg.args.def_method = cpu_to_be32(cfg->def_method);
	msg.args.codec_type = cpu_to_be32(cfg->codec);
	msg.args.snd_method = cpu_to_be32(cfg->snd_method);
	msg.args.cb_func = cpu_to_be32(0x11111111);
	msg.args.client_data = cpu_to_be32((int)am);

	msm_rpc_setup_req(&msg.hdr, AUDMGR_PROG, amg->rpc_version,
			  AUDMGR_ENABLE_CLIENT);

	rc = msm_rpc_write(amg->ept, &msg, sizeof(msg));
	if (rc < 0)
		return rc;

	rc = wait_event_timeout(am->wait, am->state != STATE_ENABLING, 15 * HZ);
	if (rc == 0) {
	    MM_AUD_ERR("tx_sample_rate:%d,rx_sample_rate:%d\n",
		    msg.args.tx_sample_rate, msg.args.rx_sample_rate);
	    MM_AUD_ERR("def_method:%d,codec_type:%d,snd_method:%d\n",
		    msg.args.def_method, msg.args.codec_type, msg.args.snd_method);
	    MM_AUD_ERR("audmgr_enable: ARM9 did not reply to RPC am->"
		    "state = %d\n", am->state);
	    dprint_int_ctrl_regs();
	    BUG();
	}

	if (am->state == STATE_ENABLED)
		return 0;

	MM_AUD_ERR("audmgr: unexpected state %d while enabling?!\n", am->state);
	return -ENODEV;
}
EXPORT_SYMBOL(audmgr_enable);

int audmgr_disable(struct audmgr *am)
{
	struct audmgr_global *amg = &the_audmgr_state;
	struct audmgr_disable_msg msg;
	int rc;

	/* Fix to get 2 way call audio */
	if (am->handle == 0xFFFF) {
		MM_AUD_ERR("audmgr_disable: without handle\n");
		return 0;
	}

	if (am->state == STATE_DISABLED)
		return 0;

	MM_AUD_INFO("audmgr_disable: session 0x%08x\n", (int) am);
	msg.handle = cpu_to_be32(am->handle);
	msm_rpc_setup_req(&msg.hdr, AUDMGR_PROG, amg->rpc_version,
			  AUDMGR_DISABLE_CLIENT);

	am->state = STATE_DISABLING;

	rc = msm_rpc_write(amg->ept, &msg, sizeof(msg));
	if (rc < 0)
		return rc;

	rc = wait_event_timeout(am->wait, am->state != STATE_DISABLING, 15 * HZ);
	if (rc == 0) {
		MM_AUD_ERR("audmgr_disable: ARM9 did not reply to RPC am->"
				"state = %d\n", am->state);
		BUG();
	}

	if (am->state == STATE_DISABLED)
		return 0;

	MM_AUD_ERR("audmgr_disable: unexpected state %d while disabling?!\n",
		am->state);
	return -ENODEV;
}
EXPORT_SYMBOL(audmgr_disable);
