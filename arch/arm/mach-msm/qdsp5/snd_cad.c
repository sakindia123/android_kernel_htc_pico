/* arch/arm/mach-msm/qdsp5/snd_cad.c
 *
 * interface to "snd" service on the baseband cpu
 * This code also borrows from snd.c, which is
 * Copyright (C) 2008 HTC Corporation
 * Copyright (c) 2009, 2012 The Linux Foundation. All rights reserved.
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
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/msm_audio.h>
#include <linux/seq_file.h>
#include <asm/atomic.h>
#include <asm/ioctls.h>
#include <mach/board.h>
#include <mach/msm_rpcrouter.h>
#include <mach/debug_mm.h>
#include <linux/debugfs.h>

struct snd_cad_ctxt {
	struct mutex lock;
	int opened;
	struct msm_rpc_endpoint *ept;
	struct msm_cad_endpoints *cad_epts;
};

struct snd_cad_sys_ctxt {
	struct mutex lock;
	struct msm_rpc_endpoint *ept;
};

struct snd_curr_dev_info {
	int rx_dev;
	int tx_dev;
};

static struct snd_cad_sys_ctxt the_snd_cad_sys;

static struct snd_cad_ctxt the_snd;
static struct snd_curr_dev_info curr_dev;

#define RPC_SND_PROG	0x30000002
#define RPC_SND_CB_PROG	0x31000002

#define RPC_SND_VERS	0x00030003

#define SND_CAD_SET_DEVICE_PROC 40
#define SND_CAD_SET_VOLUME_PROC 39
#define MAX_SND_ACTIVE_DEVICE 2

//LGE_CHANGE_S, [youngbae.choi@lge.com] , 2011-12-08
#if defined (CONFIG_MACH_LGE)
#define SND_SET_LOOPBACK_MODE_PROC 61
/* LGE_CHANGE_S :  2011-12-30, gt.kim@lge.com, Description: Bluetooth NERC Cmd Support */
#define SND_SET_NREC_PROC 77
/* LGE_CHANGE_E :  Bluetooth NERC Cmd Support*/
/* LGE_CHANGE_S :  2012-01-26, gt.kim@lge.com, Description:  Display Service Type */
#define SND_GET_SERVICE_TYPE_PROC	78
/* LGE_CHANGE_E :   Display Service Type*/
#endif
//LGE_CHANGE_E, [youngbae.choi@lge.com] , 2011-12-08
struct rpc_cad_set_device_args {
	struct cad_devices_type device;
	uint32_t ear_mute;
	uint32_t mic_mute;

	uint32_t cb_func;
	uint32_t client_data;
};

struct rpc_cad_set_volume_args {
	struct cad_devices_type device;
	uint32_t method;
	uint32_t volume;

	uint32_t cb_func;
	uint32_t client_data;
};

struct snd_cad_set_device_msg {
	struct rpc_request_hdr hdr;
	struct rpc_cad_set_device_args args;
};

struct snd_cad_set_volume_msg {
	struct rpc_request_hdr hdr;
	struct rpc_cad_set_volume_args args;
};

struct cad_endpoint *get_cad_endpoints(int *size);
//LGE_CHANGE_S, [youngbae.choi@lge.com] , 2011-12-08
#if defined (CONFIG_MACH_LGE)
struct snd_set_loopback_param_rep {
	struct rpc_reply_hdr hdr;
	uint32_t get_mode;
} lrep_cad;

struct rpc_snd_set_loopback_mode_args {
	uint32_t mode;
	uint32_t cb_func;
	uint32_t client_data;
};

struct snd_set_loopback_mode_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_set_loopback_mode_args args;
};

/* LGE_CHANGE_S :  2012-1-2, gt.kim@lge.com, Description: Bluetooth NERC Cmd Support */
struct rpc_snd_set_bt_nerc_mode_args {
	uint32_t mode;
	uint32_t cb_func;
	uint32_t client_data;
};

struct snd_set_bt_nerc_mode_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_set_bt_nerc_mode_args args;
};

struct snd_set_bt_nerc_mode_rep {
	struct rpc_reply_hdr hdr;
	uint32_t get_mode;
}bt_nerc_rep;

/* LGE_CHANGE_E :  Bluetooth NERC Cmd Support */

/* LGE_CHANGE_S :  2012-01-26, gt.kim@lge.com, Description:  Display Service Type */
int service_type;
struct rpc_snd_get_service_type_args {
	uint32_t cb_func;
	uint32_t client_data;
};

struct snd_get_service_type_msg{
	struct rpc_request_hdr hdr;
	struct rpc_snd_get_service_type_args args;
};

struct snd_get_service_type_rep {
	struct rpc_reply_hdr hdr;
	uint32_t get_service;
}get_service_rep;
/* LGE_CHANGE_E :   Display Service Type*/

union snd_set_union_param_msg{
	struct snd_set_loopback_mode_msg lbmsg;
/* LGE_CHANGE_S :  2012-1-2, gt.kim@lge.com, Description: Bluetooth NERC Cmd Support */
	struct snd_set_bt_nerc_mode_msg bt_nerc;
/* LGE_CHANGE_E :  Bluetooth NERC Cmd Support */

/* LGE_CHANGE_S :  2012-01-26, gt.kim@lge.com, Description:  Display Service Type */
	struct snd_get_service_type_msg get_service;
/* LGE_CHANGE_E :   Display Service Type*/
};

#endif
//LGE_CHANGE_E, [youngbae.choi@lge.com] , 2011-12-08

#ifdef CONFIG_DEBUG_FS
static struct dentry *dentry;

static int rtc_getdevice_dbg_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	MM_INFO("debug intf %s\n", (char *) file->private_data);
	return 0;
}

static ssize_t rtc_getdevice_dbg_read(struct file *file, char __user *buf,
			  size_t count, loff_t *ppos)
{
	int n = 0;
	static char *buffer;
	static char *swap_buf;
	const int debug_bufmax = 1024;
	int swap_count = 0;
	int rc = 0;
	int dev_count = 0;
	int dev_id = 0;
	struct msm_cad_endpoints *msm_cad_epts = 0;
	struct cad_endpoint *cad_epts;

	buffer = kmalloc(sizeof(char) * 1024, GFP_KERNEL);
	if (buffer == NULL) {
		MM_ERR("Memory allocation failed for buffer failed\n");
		return -EFAULT;
	}

	swap_buf = kmalloc(sizeof(char) * 1024, GFP_KERNEL);
	if (swap_buf == NULL) {
		MM_ERR("Memory allocation failed for swap buffer failed\n");
		kfree(buffer);
		return -EFAULT;
	}

	if (msm_cad_epts->num <= 0) {
		dev_count = 0;
		n = scnprintf(buffer, debug_bufmax, "DEV_NO:0x%x\n",
				msm_cad_epts->num);
	} else {
		for (dev_id = 0; dev_id < msm_cad_epts->num; dev_id++) {
			cad_epts = &msm_cad_epts->endpoints[dev_id];
			if (IS_ERR(cad_epts)) {
				MM_ERR("invalid snd endpoint for dev_id %d\n",
					dev_id);
				rc = PTR_ERR(cad_epts);
				continue;
			}

			if ((cad_epts->id != curr_dev.tx_dev) &&
				(cad_epts->id != curr_dev.rx_dev))
				continue;

			n += scnprintf(swap_buf + n, debug_bufmax - n,
					"ACDB_ID:0x%x;CAPB:0x%x\n",
					cad_epts->id,
					cad_epts->capability);
			dev_count++;
			MM_DBG("RTC Get Device %x Capb %x Dev Count %x\n",
					dev_id, cad_epts->capability,
					dev_count);

		}
	}
	swap_count = scnprintf(buffer, debug_bufmax, \
			"DEV_NO:0x%x\n", dev_count);

	memcpy(buffer+swap_count, swap_buf, n*sizeof(char));
	n = n+swap_count;

	buffer[n] = 0;
	rc =  simple_read_from_buffer(buf, count, ppos, buffer, n);
	kfree(buffer);
	kfree(swap_buf);
	return rc;
}

static const struct file_operations rtc_acdb_debug_fops = {
	.open = rtc_getdevice_dbg_open,
	.read = rtc_getdevice_dbg_read
};

static int rtc_debugfs_create_entry(void)
{
	int rc = 0;
	char name[sizeof "rtc_get_device"+1];

	snprintf(name, sizeof name, "rtc_get_device");
	dentry = debugfs_create_file(name, S_IFREG | S_IRUGO,
			NULL, NULL, &rtc_acdb_debug_fops);
	if (IS_ERR(dentry)) {
		MM_ERR("debugfs_create_file failed\n");
		rc = PTR_ERR(dentry);
	}
	return rc;
}
#else
static int rtc_debugfs_create_entry()
{
	return 0;
}
#endif

static inline int check_mute(int mute)
{
	return (mute == SND_MUTE_MUTED ||
		mute == SND_MUTE_UNMUTED) ? 0 : -EINVAL;
}

static int get_endpoint(struct snd_cad_ctxt *snd, unsigned long arg)
{
	int rc = 0, index;
	struct msm_cad_endpoint ept;

	if (copy_from_user(&ept, (void __user *)arg, sizeof(ept))) {
		MM_ERR("cad_ioctl get endpoint: invalid read pointer\n");
		return -EFAULT;
	}

	index = ept.id;
	if (index < 0 || index >= snd->cad_epts->num) {
		MM_ERR("snd_ioctl get endpoint: invalid index!\n");
		return -EINVAL;
	}

	ept.id = snd->cad_epts->endpoints[index].id;
	strlcpy(ept.name,
		snd->cad_epts->endpoints[index].name,
		sizeof(ept.name));

	if (copy_to_user((void __user *)arg, &ept, sizeof(ept))) {
		MM_ERR("snd_ioctl get endpoint: invalid write pointer\n");
		rc = -EFAULT;
	}

	return rc;
}

static long snd_cad_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	struct snd_cad_set_device_msg dmsg;
	struct snd_cad_set_volume_msg vmsg;

	struct msm_cad_device_config dev;
	struct msm_cad_volume_config vol;
	struct snd_cad_ctxt *snd = file->private_data;
//LGE_CHANGE_S, [youngbae.choi@lge.com] , 2011-12-08
#if defined (CONFIG_MACH_LGE)
	struct msm_snd_set_loopback_mode_param loopback;
/* LGE_CHANGE_S :  2012-01-05, gt.kim@lge.com, Decription: Bluetooth NERC Cmd Support */	
    struct msm_snd_set_bt_nerc_param bt_nerc;
/* LGE_CHANGE_E :Bluetooth NERC Cmd Support*/	
	union snd_set_union_param_msg umsg;
#endif
/*LGE_CHANBE_E : jaz.john@lge.com kernel3.0 porting based on kernel2.6.38*/
	int rc = 0;

	mutex_lock(&snd->lock);
	switch (cmd) {
	case SND_SET_DEVICE:
		if (copy_from_user(&dev, (void __user *) arg, sizeof(dev))) {
			MM_ERR("set device: invalid pointer\n");
			rc = -EFAULT;
			break;
		}

		dmsg.args.device.rx_device = cpu_to_be32(dev.device.rx_device);
		dmsg.args.device.tx_device = cpu_to_be32(dev.device.tx_device);
		dmsg.args.device.pathtype = cpu_to_be32(dev.device.pathtype);
		dmsg.args.ear_mute = cpu_to_be32(dev.ear_mute);
		dmsg.args.mic_mute = cpu_to_be32(dev.mic_mute);
		if (check_mute(dev.ear_mute) < 0 ||
				check_mute(dev.mic_mute) < 0) {
			MM_ERR("set device: invalid mute status\n");
			rc = -EINVAL;
			break;
		}
		dmsg.args.cb_func = -1;
		dmsg.args.client_data = 0;
		curr_dev.tx_dev = dev.device.tx_device;
		curr_dev.rx_dev = dev.device.rx_device;
		MM_ERR("snd_cad_set_device %d %d %d %d\n", dev.device.rx_device,
			dev.device.tx_device, dev.ear_mute, dev.mic_mute);

		rc = msm_rpc_call(snd->ept,
			SND_CAD_SET_DEVICE_PROC,
			&dmsg, sizeof(dmsg), 5 * HZ);
		break;

	case SND_SET_VOLUME:
		if (copy_from_user(&vol, (void __user *) arg, sizeof(vol))) {
			MM_ERR("set volume: invalid pointer\n");
			rc = -EFAULT;
			break;
		}

		vmsg.args.device.rx_device = cpu_to_be32(dev.device.rx_device);
		vmsg.args.device.tx_device = cpu_to_be32(dev.device.tx_device);
		vmsg.args.method = cpu_to_be32(vol.method);
		if (vol.method != SND_METHOD_VOICE &&
				vol.method != SND_METHOD_MIDI) {
			MM_ERR("set volume: invalid method %d\n", vol.method);
			rc = -EINVAL;
			break;
		}

		vmsg.args.volume = cpu_to_be32(vol.volume);
		vmsg.args.cb_func = -1;
		vmsg.args.client_data = 0;

		MM_ERR("snd_cad_set_volume %d %d %d %d\n", vol.device.rx_device,
				vol.device.tx_device, vol.method, vol.volume);

		rc = msm_rpc_call(snd->ept,
			SND_CAD_SET_VOLUME_PROC,
			&vmsg, sizeof(vmsg), 5 * HZ);

		break;

	case SND_GET_NUM_ENDPOINTS:
		if (copy_to_user((void __user *)arg,
				&snd->cad_epts->num, sizeof(unsigned))) {
			MM_ERR("get endpoint: invalid pointer\n");
			rc = -EFAULT;
		}
		break;

	case SND_GET_ENDPOINT:
		rc = get_endpoint(snd, arg);
		break;

/*LGE_CHANBE_S : seven.kim@lge.com kernel3.0 porting based on kernel2.6.38*/
#if defined (CONFIG_MACH_LGE)
	case SND_SET_LOOPBACK_MODE:
		if (copy_from_user(&loopback, (void __user *) arg, sizeof(loopback))) {
			pr_err("snd_ioctl set_loopback_mode: invalid pointer.\n");
			rc = -EFAULT;
			break;
		}

		umsg.lbmsg.args.mode = cpu_to_be32(loopback.mode);
		umsg.lbmsg.args.cb_func = -1;
		umsg.lbmsg.args.client_data = 0;



		rc = msm_rpc_call(snd->ept,
			SND_SET_LOOPBACK_MODE_PROC,
			&umsg.lbmsg, sizeof(umsg.lbmsg), 5 * HZ);

		if (rc < 0) {
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		} else {
			loopback.get_param = be32_to_cpu(lrep_cad.get_mode);
			printk(KERN_INFO "%s:loopback mode ->%d\n", __func__, loopback.get_param);
			if (copy_to_user((void __user *)arg, &loopback, sizeof(loopback))) {
				pr_err("snd_ioctl get loopback mode: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
	break;

/* LGE_CHANGE_S :  2012-01-26, gt.kim@lge.com, Description:  Display Service Type */
	case SND_GET_SERVICE_TYPE:
		umsg.get_service.args.cb_func = -1;
		umsg.get_service.args.client_data = 0;

		pr_info("get_service~~ \n");

		rc = msm_rpc_call_reply(snd->ept,
			SND_GET_SERVICE_TYPE_PROC,
			&umsg.get_service, sizeof(umsg.get_service),&get_service_rep, sizeof(get_service_rep), 5 * HZ);

		if (rc < 0){
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		}
		else
		{
			service_type = be32_to_cpu(get_service_rep.get_service);
			printk(KERN_INFO "%s:get Service ->%d\n", __func__, service_type);
			if (copy_to_user((void __user *)arg, &service_type, sizeof(service_type))) {
				pr_err("snd_ioctl get service: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;
/* LGE_CHANGE_E :	Display Service Type*/
/* LGE_CHANGE_S :  2011-12-30, gt.kim@lge.com, Description: Bluetooth NERC Cmd Support */
case SND_SET_NREC:
	if (copy_from_user(&bt_nerc, (void __user *) arg, sizeof(bt_nerc))) {
		pr_err("snd_ioctl set_NREC: invalid pointer.\n");
		rc = -EFAULT;
		break;
	}

    umsg.bt_nerc.args.mode          = cpu_to_be32(bt_nerc.mode);
    umsg.bt_nerc.args.cb_func       = -1;
    umsg.bt_nerc.args.client_data   = 0;

	pr_info("set_NREC %d \n", bt_nerc.mode);

	rc = msm_rpc_call_reply(snd->ept,
		SND_SET_NREC_PROC,
		&umsg.bt_nerc, sizeof(umsg.bt_nerc),&bt_nerc_rep, sizeof(bt_nerc_rep), 5 * HZ);

	if (rc < 0){
		printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
	}
	else
	{
		bt_nerc.get_param = be32_to_cpu(bt_nerc_rep.get_mode);
		printk(KERN_INFO "%s:NREC mode ->%d\n", __func__, bt_nerc.get_param);
		if (copy_to_user((void __user *)arg, &bt_nerc, sizeof(bt_nerc))) {
			pr_err("snd_ioctl get NREC mode: invalid write pointer.\n");
			rc = -EFAULT;
		}
	}

	break;
/* LGE_CHANGE_E :  Bluetooth NERC Cmd Support */
#endif
/*LGE_CHANBE_E : seven.kim@lge.com kernel3.0 porting based on kernel2.6.38*/


	default:
		MM_ERR("unknown command\n");
		rc = -EINVAL;
		break;
	}
	mutex_unlock(&snd->lock);

	return rc;
}

static int snd_cad_release(struct inode *inode, struct file *file)
{
	struct snd_cad_ctxt *snd = file->private_data;
	int rc;

	mutex_lock(&snd->lock);
	rc = msm_rpc_close(snd->ept);
	if (rc < 0)
		MM_ERR("msm_rpc_close failed\n");
	snd->ept = NULL;
	snd->opened = 0;
	mutex_unlock(&snd->lock);
	return 0;
}
static int snd_cad_sys_release(void)
{
	struct snd_cad_sys_ctxt *snd_cad_sys = &the_snd_cad_sys;
	int rc = 0;

	mutex_lock(&snd_cad_sys->lock);
	rc = msm_rpc_close(snd_cad_sys->ept);
	if (rc < 0)
		MM_ERR("msm_rpc_close failed\n");
	snd_cad_sys->ept = NULL;
	mutex_unlock(&snd_cad_sys->lock);
	return rc;
}
static int snd_cad_open(struct inode *inode, struct file *file)
{
	struct snd_cad_ctxt *snd = &the_snd;
	int rc = 0;

	mutex_lock(&snd->lock);
	if (snd->opened == 0) {
		if (snd->ept == NULL) {
			snd->ept = msm_rpc_connect_compatible(RPC_SND_PROG,
					RPC_SND_VERS, 0);
			if (IS_ERR(snd->ept)) {
				rc = PTR_ERR(snd->ept);
				snd->ept = NULL;
				MM_ERR("cad connect failed with VERS %x\n",
					RPC_SND_VERS);
				goto err;
			}
		}
		file->private_data = snd;
		snd->opened = 1;
	} else {
		MM_ERR("snd already opened\n");
		rc = -EBUSY;
	}

err:
	mutex_unlock(&snd->lock);
	return rc;
}
static int snd_cad_sys_open(void)
{
	struct snd_cad_sys_ctxt *snd_cad_sys = &the_snd_cad_sys;
	int rc = 0;

	mutex_lock(&snd_cad_sys->lock);
	if (snd_cad_sys->ept == NULL) {
		snd_cad_sys->ept = msm_rpc_connect_compatible(RPC_SND_PROG,
			RPC_SND_VERS, 0);
		if (IS_ERR(snd_cad_sys->ept)) {
			rc = PTR_ERR(snd_cad_sys->ept);
			snd_cad_sys->ept = NULL;
			MM_ERR("func %s : cad connect failed with VERS %x\n",
				__func__, RPC_SND_VERS);
			goto err;
		}
	} else
		MM_DBG("snd already opened\n");
err:
	mutex_unlock(&snd_cad_sys->lock);
	return rc;
}

static const struct file_operations snd_cad_fops = {
	.owner		= THIS_MODULE,
	.open		= snd_cad_open,
	.release	= snd_cad_release,
	.unlocked_ioctl	= snd_cad_ioctl,
};

struct miscdevice snd_cad_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "msm_cad",
	.fops	= &snd_cad_fops,
};

static long snd_cad_vol_enable(const char *arg)
{
	struct snd_cad_sys_ctxt *snd_cad_sys = &the_snd_cad_sys;
	struct snd_cad_set_volume_msg vmsg;
	struct msm_cad_volume_config vol;
	int rc = 0;

	rc = sscanf(arg, "%d %d %d %d", &vol.device.rx_device,
			&vol.device.tx_device, &vol.method, &vol.volume);
	if (rc != 4) {
		MM_ERR("Invalid arguments. Usage: <rx_device> <tx_device>" \
			"method> <volume>\n");
		rc = -EINVAL;
		return rc;
	}

	vmsg.args.device.rx_device = cpu_to_be32(vol.device.rx_device);
	vmsg.args.device.tx_device = cpu_to_be32(vol.device.tx_device);
	vmsg.args.method = cpu_to_be32(vol.method);
	if (vol.method != SND_METHOD_VOICE && vol.method != SND_METHOD_MIDI) {
		MM_ERR("snd_cad_ioctl set volume: invalid method\n");
		rc = -EINVAL;
		return rc;
	}

	vmsg.args.volume = cpu_to_be32(vol.volume);
	vmsg.args.cb_func = -1;
	vmsg.args.client_data = 0;

	MM_DBG("snd_cad_set_volume %d %d %d %d\n", vol.device.rx_device,
			vol.device.tx_device, vol.method, vol.volume);

	rc = msm_rpc_call(snd_cad_sys->ept,
		SND_CAD_SET_VOLUME_PROC,
		&vmsg, sizeof(vmsg), 5 * HZ);
	return rc;
}

static long snd_cad_dev_enable(const char *arg)
{
	struct snd_cad_sys_ctxt *snd_cad_sys = &the_snd_cad_sys;
	struct snd_cad_set_device_msg dmsg;
	struct msm_cad_device_config dev;
	int rc = 0;


	rc = sscanf(arg, "%d %d %d %d", &dev.device.rx_device,
			&dev.device.tx_device, &dev.ear_mute, &dev.mic_mute);
	if (rc != 4) {
		MM_ERR("Invalid arguments. Usage: <rx_device> <tx_device> "\
			"<ear_mute> <mic_mute>\n");
		rc = -EINVAL;
		return rc;
	}
	dmsg.args.device.rx_device = cpu_to_be32(dev.device.rx_device);
	dmsg.args.device.tx_device = cpu_to_be32(dev.device.tx_device);
	dmsg.args.device.pathtype = cpu_to_be32(CAD_DEVICE_PATH_RX_TX);
	dmsg.args.ear_mute = cpu_to_be32(dev.ear_mute);
	dmsg.args.mic_mute = cpu_to_be32(dev.mic_mute);
	if (check_mute(dev.ear_mute) < 0 ||
			check_mute(dev.mic_mute) < 0) {
		MM_ERR("snd_cad_ioctl set device: invalid mute status\n");
		rc = -EINVAL;
		return rc;
	}
	dmsg.args.cb_func = -1;
	dmsg.args.client_data = 0;
	curr_dev.tx_dev = dev.device.tx_device;
	curr_dev.rx_dev = dev.device.rx_device;

	MM_INFO("snd_cad_set_device %d %d %d %d\n", dev.device.rx_device,
			dev.device.tx_device, dev.ear_mute, dev.mic_mute);

	rc = msm_rpc_call(snd_cad_sys->ept,
		SND_CAD_SET_DEVICE_PROC,
		&dmsg, sizeof(dmsg), 5 * HZ);
	return rc;
}

static ssize_t snd_cad_dev_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_cad_sys_ctxt *snd_cad_sys = &the_snd_cad_sys;
	int rc = 0;

	rc = snd_cad_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_cad_sys->lock);
	status = snd_cad_dev_enable(buf);
	mutex_unlock(&snd_cad_sys->lock);

	rc = snd_cad_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static ssize_t snd_cad_vol_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_cad_sys_ctxt *snd_cad_sys = &the_snd_cad_sys;
	int rc = 0;

	rc = snd_cad_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_cad_sys->lock);
	status = snd_cad_vol_enable(buf);
	mutex_unlock(&snd_cad_sys->lock);

	rc = snd_cad_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static DEVICE_ATTR(device, S_IWUSR | S_IRUGO,
		NULL, snd_cad_dev_store);

static DEVICE_ATTR(volume, S_IWUSR | S_IRUGO,
		NULL, snd_cad_vol_store);

static int snd_cad_probe(struct platform_device *pdev)
{
	struct snd_cad_ctxt *snd = &the_snd;
	struct snd_cad_sys_ctxt *snd_cad_sys = &the_snd_cad_sys;
	int rc = 0;

	mutex_init(&snd->lock);
	mutex_init(&snd_cad_sys->lock);
	snd_cad_sys->ept = NULL;
	snd->cad_epts =
			(struct msm_cad_endpoints *)pdev->dev.platform_data;
	rc = misc_register(&snd_cad_misc);
	if (rc)
		return rc;

	rc = device_create_file(snd_cad_misc.this_device, &dev_attr_device);
	if (rc) {
		misc_deregister(&snd_cad_misc);
		return rc;
	}

	rc = device_create_file(snd_cad_misc.this_device, &dev_attr_volume);
	if (rc) {
		device_remove_file(snd_cad_misc.this_device,
						&dev_attr_device);
		misc_deregister(&snd_cad_misc);
	}

#ifdef CONFIG_DEBUG_FS
	rc = rtc_debugfs_create_entry();
	if (rc) {
		device_remove_file(snd_cad_misc.this_device,
						&dev_attr_volume);
		device_remove_file(snd_cad_misc.this_device,
						&dev_attr_device);
		misc_deregister(&snd_cad_misc);
	}
#endif
	return rc;
}

static struct platform_driver snd_cad_plat_driver = {
	.probe = snd_cad_probe,
	.driver = {
		.name = "msm_cad",
		.owner = THIS_MODULE,
	},
};

static int __init snd_cad_init(void)
{
	return platform_driver_register(&snd_cad_plat_driver);
}

module_init(snd_cad_init);

MODULE_DESCRIPTION("MSM CAD SND driver");
MODULE_LICENSE("GPL v2");
