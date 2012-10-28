/* arch/arm/mach-msm/htc_acoustic.c
 *
 * Copyright (C) 2007-2008 HTC Corporation
 * Author: Laurence Chen <Laurence_Chen@htc.com>
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
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include <mach/msm_smd.h>
#include <mach/msm_rpcrouter.h>
#include <mach/msm_iomap.h>
#include <mach/htc_headset_mgr.h>
#include <asm/cacheflush.h>

#include "smd_private.h"
#include <mach/htc_acoustic.h>

#define ACOUSTIC_IOCTL_MAGIC 'p'
#define ACOUSTIC_ARM11_DONE _IOW(ACOUSTIC_IOCTL_MAGIC, 22, unsigned int)
#define ACOUSTIC_ALLOC_SMEM _IOW(ACOUSTIC_IOCTL_MAGIC, 23, unsigned int)
#define SET_VR_MODE	        _IOW(ACOUSTIC_IOCTL_MAGIC, 24, unsigned int)
#define ACOUSTIC_SET_HAC _IOW(ACOUSTIC_IOCTL_MAGIC, 25, unsigned int)
#define ACOUSTIC_SET_FM_VOLUME _IOW(ACOUSTIC_IOCTL_MAGIC, 26, unsigned int)
#define ACOUSTIC_SET_TX_MUTE _IOW(ACOUSTIC_IOCTL_MAGIC, 27, unsigned int)
#define ACOUSTIC_ENABLE_BEATS _IOW(ACOUSTIC_IOCTL_MAGIC, 28, unsigned int)
#define ACOUSTIC_ENABLE_SH _IOW(ACOUSTIC_IOCTL_MAGIC, 29, unsigned int)
#define ACOUSTIC_SET_CDMA_MUTE _IOW(ACOUSTIC_IOCTL_MAGIC, 30, unsigned int)
#define ACOUSTIC_SET_BEATS_CONFIG _IOW(ACOUSTIC_IOCTL_MAGIC, 31, unsigned int)
#define ACOUSTIC_GET_TABLES 	_IOW(ACOUSTIC_IOCTL_MAGIC, 32, unsigned)

#define HTCRPOG	0x30100002
#define HTCVERS 0
#if defined(CONFIG_ARCH_MSM7X27A)
#define HTC_ACOUSTIC_TABLE_SIZE        (0x20000)
#else
#define HTC_ACOUSTIC_TABLE_SIZE        (0x10000)
#endif
#define ONCRPC_SET_MIC_BIAS_PROC       (1)
#define ONCRPC_ACOUSTIC_INIT_PROC      (5)
#define ONCRPC_ALLOC_ACOUSTIC_MEM_PROC (6)
#define ONCRPC_MUTE_TX_VOL_PROC        (8)
#define ONCRPC_ENABLE_VR_MODE          (9)
#define ONCRPC_SET_FM_VOLUME_PROC      (10)
#define ONCRPC_ENABLE_BEATS_PROC       (11)
#define ONCRPC_ENABLE_SH_PROC          (12)
#define ONCRPC_MUTE_CDMA_PROC          (13)
#define ONCRPC_SET_BEATS_CONFIG_PROC   (14)

struct set_smem_req {
	struct rpc_request_hdr hdr;
	uint32_t size;
};

struct set_smem_rep {
	struct rpc_reply_hdr hdr;
	int n;
};

struct set_acoustic_req {
	struct rpc_request_hdr hdr;
} req;

struct set_acoustic_rep {
	struct rpc_reply_hdr hdr;
	int n;
} rep;

static uint32_t htc_acoustic_phy_addr;
static uint32_t htc_acoustic_vir_addr = 0;
static struct msm_rpc_endpoint *endpoint;
struct mutex rpc_connect_mutex;
struct class *htc_class;
struct mutex acoustic_lock;
static int hac_enable_flag;
static void *f_table;
static int f_smem;
static int hac_smem;
#if defined(CONFIG_ARCH_MSM7X27A)
static int table_offset = 0x00001600 + 0x00001600;
static int table_size = 6 * 285 * sizeof(unsigned short);
#else
static int table_offset = 0x00000C00 + 0x00000C00;
static int table_size = 6 * 160 * sizeof(unsigned short);
#endif
static int swap_f_table(int);
static int first_time = 1;
static struct acoustic_ops default_acoustic_ops;
static struct acoustic_ops * the_ops = &default_acoustic_ops;

void acoustic_register_ops(struct acoustic_ops *ops)
{
	the_ops = ops;
}

static int is_rpc_connect(void)
{
	mutex_lock(&rpc_connect_mutex);
	if (endpoint == NULL) {
		endpoint = msm_rpc_connect_compatible(HTCRPOG,HTCVERS, 0);
		if (IS_ERR(endpoint)) {
			pr_aud_err("%s: init rpc failed!(msm_rpc_connect_compatible)\n",
				__func__);

			endpoint = msm_rpc_connect(HTCRPOG, HTCVERS, 0);
			if (IS_ERR(endpoint)) {
				pr_aud_err("%s: init rpc failed!(msm_rpc_connect) rc = %ld\n",
					__func__, PTR_ERR(endpoint));
				endpoint = NULL;
				mutex_unlock(&rpc_connect_mutex);
				return -1;
			}
		}
	}
	mutex_unlock(&rpc_connect_mutex);
	return 0;
}

int enable_mic_bias(int on)
{
	struct mic_bias_req {
		struct rpc_request_hdr hdr;
		uint32_t on;
	} req;

	if (is_rpc_connect() == -1)
		return -EIO;

	if (the_ops->enable_mic_bias)
		the_ops->enable_mic_bias(on, 1);

	req.on = cpu_to_be32(on);
	return msm_rpc_call(endpoint, ONCRPC_SET_MIC_BIAS_PROC,
			    &req, sizeof(req), 5 * HZ);
}

int enable_mos_test(int enable)
{
	/* Do nothing */
	return 0;
}
EXPORT_SYMBOL(enable_mos_test);

static int swap_f_table(int enable)
{
	int i = 0;
	if (enable) {
		if (first_time) {
			memcpy(f_table, (void *)(f_smem), table_size);
			first_time = 0;
		}

#if defined(CONFIG_ARCH_MSM7X27A)
		for (i = 0; i < 6; i++) {
			memcpy((void *)(f_smem + (i * 285 * sizeof(uint16_t))),
				(void *)(hac_smem) , (285 * sizeof(uint16_t)));
		}
#else
		for (i = 0; i < 6; i++) {
			memcpy((void *)(f_smem + (i * 160 * sizeof(uint16_t))),
				(void *)(hac_smem) , (160 * sizeof(uint16_t)));
		}
#endif
	} else {
		memcpy((void *)f_smem, f_table, table_size);

	}
	return 0;
}

static int acoustic_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long pgoff;
	size_t size = vma->vm_end - vma->vm_start;
	if (vma->vm_pgoff != 0)
		return -EINVAL;

	if (size <= HTC_ACOUSTIC_TABLE_SIZE)
		pgoff = htc_acoustic_phy_addr >> PAGE_SHIFT;
	else
		return -EINVAL;

	vma->vm_flags |= VM_IO | VM_RESERVED;

	if (io_remap_pfn_range(vma, vma->vm_start, pgoff,
			       size, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

static int acoustic_open(struct inode *inode, struct file *file)
{
	int rc, reply_value;
	struct set_smem_req req_smem;
	struct set_smem_rep rep_smem;

	if (is_rpc_connect() == -1)
		return -EIO;

	if (!htc_acoustic_vir_addr) {
		req_smem.size = cpu_to_be32(HTC_ACOUSTIC_TABLE_SIZE);
		rc = msm_rpc_call_reply(endpoint,
					ONCRPC_ALLOC_ACOUSTIC_MEM_PROC,
					&req_smem, sizeof(req_smem),
					&rep_smem, sizeof(rep_smem),
					5 * HZ);

		reply_value = be32_to_cpu(rep_smem.n);
		if (reply_value != 0 || rc < 0) {
			pr_aud_err("ALLOC_ACOUSTIC_MEM_PROC failed %d.\n", rc);
			return rc;
		}

		htc_acoustic_vir_addr =
				(uint32_t)smem_alloc(SMEM_ID_VENDOR1,
						HTC_ACOUSTIC_TABLE_SIZE);
		htc_acoustic_phy_addr = MSM_SHARED_RAM_PHYS +
					(htc_acoustic_vir_addr -
						(uint32_t)MSM_SHARED_RAM_BASE);
		htc_acoustic_phy_addr = ((htc_acoustic_phy_addr + 4095) & ~4095);
		htc_acoustic_vir_addr = ((htc_acoustic_vir_addr + 4095) & ~4095);

		if (htc_acoustic_phy_addr <= 0) {
			pr_aud_err("htc_acoustic_phy_addr wrong.\n");
			return -EFAULT;
		}

#if defined(CONFIG_ARCH_MSM7X27A)
		f_smem = htc_acoustic_vir_addr + table_offset +
				(12 * 285 * sizeof(unsigned short));
		hac_smem = htc_acoustic_vir_addr + table_offset +
				(27 * 285 * sizeof(unsigned short));
#else
		f_smem = htc_acoustic_vir_addr + table_offset +
				(12 * 160 * sizeof(unsigned short));
		hac_smem = htc_acoustic_vir_addr + table_offset +
				(27 * 160 * sizeof(unsigned short));
#endif
	}
	return 0;
}

static int acoustic_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long acoustic_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	int rc = -1, reply_value;
	int vr_arg, hac_arg, mute_arg, beats_arg, sh_arg;
	int cdma_mute_arg, beats_cfg_arg;
	uint32_t level;

	struct vr_mode_req {
		struct rpc_request_hdr hdr;
		uint32_t enable;
	} vr_req;

	struct fm_vol_req {
		struct rpc_request_hdr hdr;
		uint32_t volume;
	} vol_req;

	struct tx_mute_req {
		struct rpc_request_hdr hdr;
		int mute;
	} mute_req, cdma_mute_req;

	struct enable_req {
		struct rpc_request_hdr hdr;
		int enable;
	} beats_req, sh_req;

	struct cfg_req {
		struct rpc_request_hdr hdr;
		int cfg;
	} beats_cfg_req;

	switch (cmd) {
	case ACOUSTIC_ARM11_DONE:
#if 0
		pr_aud_info("ioctl ACOUSTIC_ARM11_DONE called %d.\n", current->pid);
#endif
#ifdef CONFIG_OUTER_CACHE
		outer_flush_range(htc_acoustic_phy_addr,
			htc_acoustic_phy_addr + HTC_ACOUSTIC_TABLE_SIZE);
#endif

		rc = msm_rpc_call_reply(endpoint,
					ONCRPC_ACOUSTIC_INIT_PROC, &req,
					sizeof(req), &rep, sizeof(rep),
					5 * HZ);

		reply_value = be32_to_cpu(rep.n);
		if (reply_value != 0 || rc < 0) {
			pr_aud_err("ONCRPC_ACOUSTIC_INIT_PROC failed %d.\n", rc);
			return reply_value;
		} else {
#if 0
			pr_aud_info("ONCRPC_ACOUSTIC_INIT_PROC success.\n");
#endif
			return 0;
		}
		break;
	case SET_VR_MODE:
		if (copy_from_user(&vr_arg, (void *)arg, sizeof(vr_arg))) {
		    rc = -EFAULT;
		    break;
		}
		vr_req.enable = cpu_to_be32(vr_arg);
		pr_aud_info("htc_acoustic set_vr_mode: %d\n", vr_arg);
		rc = msm_rpc_call(endpoint, ONCRPC_ENABLE_VR_MODE,
			&vr_req, sizeof(vr_req), 5*HZ);
		if (rc < 0)
			pr_aud_err("ONCRPC_ENABLE_VR_MODE failed %d.\n", rc);
		break;
	case ACOUSTIC_SET_HAC:
		if (copy_from_user(&hac_arg, (void *)arg, sizeof(hac_arg))) {
			rc = -EFAULT;
			break;
		}
		mutex_lock(&acoustic_lock);
		if (hac_arg == 1) {
			if (hac_enable_flag == 0) {
				swap_f_table(1);
				pr_aud_info("Enable HAC\n");
			}
			hac_enable_flag = 1;
		}
		else {
			if (hac_enable_flag == 1) {
				swap_f_table(0);
				pr_aud_info("Disable HAC\n");
			}
			hac_enable_flag = 0;
		}
		mutex_unlock(&acoustic_lock);
		rc = 0;
		break;
	case ACOUSTIC_SET_FM_VOLUME:
		if (copy_from_user(&level, (void *)arg, sizeof(level))) {
		    rc = -EFAULT;
		    break;
		}
		vol_req.volume = cpu_to_be32(level);
		pr_aud_info("htc_acoustic set_fm_volume: level %d\n", level);
		rc = msm_rpc_call(endpoint, ONCRPC_SET_FM_VOLUME_PROC,
			&vol_req, sizeof(vol_req), 5 * HZ);
		if (rc < 0)
			pr_aud_err("ONCRPC_SET_FM_VOLUME_PROC failed %d.\n", rc);
		break;
	case ACOUSTIC_SET_TX_MUTE:
		if (copy_from_user(&mute_arg, (void *)arg, sizeof(mute_arg))) {
		    rc = -EFAULT;
		    break;
		}
		mute_req.mute = cpu_to_be32(mute_arg);
		pr_aud_info("htc_acoustic set_tx_mute: %d\n", mute_arg);
		rc = msm_rpc_call(endpoint, ONCRPC_MUTE_TX_VOL_PROC,
			&mute_req, sizeof(mute_req), 5*HZ);
		if (rc < 0)
			pr_aud_err("ONCRPC_MUTE_TX_VOL_PROC failed %d.\n", rc);
		break;
	case ACOUSTIC_ENABLE_BEATS:
		if (copy_from_user(&beats_arg, (void *)arg, sizeof(beats_arg))) {
		    rc = -EFAULT;
		    break;
		}

		if (the_ops->enable_beats) {
			rc = the_ops->enable_beats(beats_arg);
			pr_aud_info("the_ops->enable_beats rc is %d\n",rc);
		}
		
		if (rc < 0) {
			beats_req.enable = cpu_to_be32(beats_arg);
			pr_aud_info("htc_acoustic enable_beats: %d\n", beats_arg);

			rc = msm_rpc_call(endpoint, ONCRPC_ENABLE_BEATS_PROC,
				&beats_req, sizeof(beats_req), 5*HZ);
			pr_aud_info("rc is %d\n",rc);
		}
		if (rc < 0)
			pr_aud_err("ONCRPC_ENABLE_BEATS_PROC failed %d.\n", rc);
		break;
	case ACOUSTIC_ENABLE_SH:
		if (copy_from_user(&sh_arg, (void *)arg, sizeof(sh_arg))) {
		    rc = -EFAULT;
		    break;
		}
		sh_req.enable = cpu_to_be32(sh_arg);
		pr_aud_info("htc_acoustic enable_soundhound: %d\n", sh_arg);
		rc = msm_rpc_call(endpoint, ONCRPC_ENABLE_SH_PROC,
			&sh_req, sizeof(sh_req), 5*HZ);
		if (rc < 0)
			pr_aud_err("ONCRPC_ENABLE_SH_PROC failed %d.\n", rc);
		break;
	case ACOUSTIC_SET_CDMA_MUTE:
		if (copy_from_user(&cdma_mute_arg, (void *)arg, sizeof(cdma_mute_arg))) {
		    rc = -EFAULT;
		    break;
		}
		cdma_mute_req.mute = cpu_to_be32(cdma_mute_arg);
		pr_aud_info("htc_acoustic set_CDMA_mute: %d\n", cdma_mute_arg);
		rc = msm_rpc_call(endpoint, ONCRPC_MUTE_CDMA_PROC,
			&cdma_mute_req, sizeof(cdma_mute_req), 5*HZ);
		if (rc < 0)
			pr_aud_err("ONCRPC_MUTE_CDMA_PROC failed %d.\n", rc);
		break;
	case ACOUSTIC_SET_BEATS_CONFIG:
		if (copy_from_user(&beats_cfg_arg, (void *)arg, sizeof(beats_cfg_arg))) {
		    rc = -EFAULT;
		    break;
		}
		beats_cfg_req.cfg = cpu_to_be32(beats_cfg_arg);
		pr_aud_info("htc_acoustic set_beats_cfg: %d\n", beats_cfg_arg);
		rc = msm_rpc_call(endpoint, ONCRPC_SET_BEATS_CONFIG_PROC,
			&beats_cfg_req, sizeof(beats_cfg_req), 5*HZ);
		if (rc < 0)
			pr_aud_err("ONCRPC_SET_BEATS_CONFIG_PROC failed %d.\n", rc);
		break;
	case ACOUSTIC_GET_TABLES: {
		if (the_ops->get_acoustic_tables) {
			struct acoustic_tables tb;
			memset(tb.acoustic, '\0', PROPERTY_VALUE_MAX);
			memset(tb.acoustic_wb, '\0', PROPERTY_VALUE_MAX);
			memset(tb.aic3254, '\0', PROPERTY_VALUE_MAX);
			memset(tb.aic3254_dsp, '\0', PROPERTY_VALUE_MAX);
			memset(tb.spkamp, '\0', PROPERTY_VALUE_MAX);
			pr_aud_info("call get_acoustic_tables!\n");
			the_ops->get_acoustic_tables(&tb);
			if (copy_to_user((void *) arg,
				&tb, sizeof(tb))) {
				pr_aud_err("acoustic_ioctl: "
					"ACOUSTIC_ACOUSTIC_GET_TABLES failed\n");
				rc = -EFAULT;
			} else
				rc = 0;
		} else
			rc = -EFAULT;
		pr_aud_info("ACOUSTIC_GET_TABLES: return %d\n", rc);
		break;
	}
	default:
		rc = -EINVAL;
	}
	return rc;
}

static struct file_operations acoustic_fops = {
	.owner = THIS_MODULE,
	.open = acoustic_open,
	.release = acoustic_release,
	.mmap = acoustic_mmap,
	.unlocked_ioctl = acoustic_ioctl,
};

static struct miscdevice acoustic_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "htc-acoustic",
	.fops = &acoustic_fops,
};

static ssize_t htc_show(struct device *dev,
				struct device_attribute *attr,  char *buf)
{

	char *s = buf;
	mutex_lock(&acoustic_lock);
	s += sprintf(s, "%d\n", hac_enable_flag);
	mutex_unlock(&acoustic_lock);
	return s - buf;

}
static ssize_t htc_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{

	if (count == (strlen("enable") + 1) &&
		strncmp(buf, "enable", strlen("enable")) == 0) {
		mutex_lock(&acoustic_lock);
		if (hac_enable_flag == 0) {
			swap_f_table(1);
			pr_aud_info("Enable HAC\n");
		}
		hac_enable_flag = 1;
		mutex_unlock(&acoustic_lock);
		return count;
	}
	if (count == (strlen("disable") + 1) &&
		strncmp(buf, "disable", strlen("disable")) == 0) {
		mutex_lock(&acoustic_lock);
		if (hac_enable_flag == 1) {
			swap_f_table(0);
			pr_aud_info("Disable HAC\n");
		}
		hac_enable_flag = 0;
		mutex_unlock(&acoustic_lock);
		return count;
	}
	pr_aud_err("hac_flag_store: invalid argument\n");
	return -EINVAL;

}

static DEVICE_ATTR(flag, 0664, htc_show, htc_store);

static int __init acoustic_init(void)
{
	int ret;

	ret = misc_register(&acoustic_misc);
	if (ret < 0) {
		pr_aud_err("failed to register misc device!\n");
		return ret;
	}
	mutex_init(&rpc_connect_mutex);

	htc_class = class_create(THIS_MODULE, "htc_acoustic");
	if (IS_ERR(htc_class)) {
		ret = PTR_ERR(htc_class);
		htc_class = NULL;
		goto err_create_class;
	}

	acoustic_misc.this_device =
			device_create(htc_class, NULL, 0 , NULL, "hac");
	if (IS_ERR(acoustic_misc.this_device)) {
		ret = PTR_ERR(acoustic_misc.this_device);
		acoustic_misc.this_device = NULL;
		goto err_create_class;
	}

	ret = device_create_file(acoustic_misc.this_device, &dev_attr_flag);
	if (ret < 0)
		goto err_create_class_device;


	f_table = kmalloc(table_size, GFP_KERNEL);
	if (f_table == NULL)
		goto err_create_class_device_file;

	mutex_init(&acoustic_lock);

#if defined(CONFIG_HTC_HEADSET_MGR)
	{
		struct headset_notifier notifier;
		notifier.id = HEADSET_REG_MIC_BIAS;
		notifier.func = enable_mic_bias;
		headset_notifier_register(&notifier);
	}
#endif

	return 0;

err_create_class_device_file:
	device_remove_file(acoustic_misc.this_device, &dev_attr_flag);
err_create_class_device:
	device_destroy(htc_class, 0);
err_create_class:
	return ret;
}

static void __exit acoustic_exit(void)
{
	int ret;

	device_remove_file(acoustic_misc.this_device, &dev_attr_flag);
	device_destroy(htc_class, 0);
	class_destroy(htc_class);

	ret = misc_deregister(&acoustic_misc);
	if (ret < 0)
		pr_aud_err("failed to unregister misc device!\n");
}

module_init(acoustic_init);
module_exit(acoustic_exit);

MODULE_AUTHOR("Laurence Chen <Laurence_Chen@htc.com>");
MODULE_DESCRIPTION("HTC acoustic driver");
MODULE_LICENSE("GPL");
