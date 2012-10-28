/* linux/arch/arm/mach-msm/board-golfu-wifi.c
*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/skbuff.h>
#include <linux/wifi_tiwlan.h>
#include <mach/htc_fast_clk.h>

#include "board-golfu.h"
#include "devices.h"
#include "proc_comm.h"
#include <mach/vreg.h>
#include <linux/gpio.h>

//QCA START (pre-allocated memory)
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>

#define ATH6KL_BOARD_DATA_FILE_SIZE   (4 * 1024)
#define ATH6K_FW_FILE_SIZE 	(124 * 1024)

#define ATH6K_FW_NUM 2

struct ath6kl_prealloc_mem_struct {
	void *mem_ptr;
	unsigned long size;
};

static struct ath6kl_prealloc_mem_struct ath6kl_fw_buf[ATH6K_FW_NUM] = {
	{ NULL, (ATH6KL_BOARD_DATA_FILE_SIZE) },
	{ NULL, (ATH6K_FW_FILE_SIZE) },
};

void *ath6kl_get_prealloc_fw_buf(u8 fw_type)
{
	memset(ath6kl_fw_buf[fw_type].mem_ptr, 0, ath6kl_fw_buf[fw_type].size);
	return ath6kl_fw_buf[fw_type].mem_ptr;
}
EXPORT_SYMBOL(ath6kl_get_prealloc_fw_buf);

static int ath6kl_prealloc_mem(void)
{
	int i;
	printk("%s\n",__func__);
	for(i = 0 ; i < ATH6K_FW_NUM; i++) {
		ath6kl_fw_buf[i].mem_ptr = kmalloc(ath6kl_fw_buf[i].size,
				GFP_KERNEL);
		if (ath6kl_fw_buf[i].mem_ptr == NULL)
			return -ENOMEM;
	}
	return 0;
}
//QCA END (pre-allocated memory)

static uint32_t wifi_on_gpio_table[] = {
	GPIO_CFG(GOLFU_GPIO_SD_D3, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), /* DAT3 */
	GPIO_CFG(GOLFU_GPIO_SD_D2, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), /* DAT2 */
	GPIO_CFG(GOLFU_GPIO_SD_D1, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), /* DAT1 */
	GPIO_CFG(GOLFU_GPIO_SD_D0, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), /* DAT0 */
	GPIO_CFG(GOLFU_GPIO_SD_CMD, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), /* CMD */
	GPIO_CFG(GOLFU_GPIO_SD_CLK_0, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_10MA), /* CLK */
	//GPIO_CFG(GOLFU_GPIO_WIFI_IRQ, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* WLAN IRQ */
	GPIO_CFG(GOLFU_GPIO_WIFI_SHUTDOWN_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* WLAN EN */
};

static uint32_t wifi_off_gpio_table[] = {
	GPIO_CFG(GOLFU_GPIO_SD_D3, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* DAT3 */
	GPIO_CFG(GOLFU_GPIO_SD_D2, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* DAT2 */
	GPIO_CFG(GOLFU_GPIO_SD_D1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* DAT1 */
	GPIO_CFG(GOLFU_GPIO_SD_D0, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* DAT0 */
	GPIO_CFG(GOLFU_GPIO_SD_CMD, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* CMD */
	GPIO_CFG(GOLFU_GPIO_SD_CLK_0, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* CLK */
	//GPIO_CFG(GOLFU_GPIO_WIFI_IRQ, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* WLAN IRQ */
	GPIO_CFG(GOLFU_GPIO_WIFI_SHUTDOWN_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* WLAN EN */
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("[WLAN] %s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

int atheros_wifi_power(int on)
{
	printk(KERN_INFO "[WLAN] %s: %d\n", __func__, on);

	if (on) {
		config_gpio_table(wifi_on_gpio_table, ARRAY_SIZE(wifi_on_gpio_table));
		msleep(200);
		gpio_set_value(GOLFU_GPIO_WIFI_SHUTDOWN_N, 1); /* WIFI_SHUTDOWN */
	} else {
		gpio_set_value(GOLFU_GPIO_WIFI_SHUTDOWN_N,0);
		msleep(1);
		config_gpio_table(wifi_off_gpio_table, ARRAY_SIZE(wifi_off_gpio_table));
		msleep(10);
	}
	msleep(250);
	printk(KERN_INFO "[WLAN] %s: ---\n", __func__);
	return 0;
}
EXPORT_SYMBOL(atheros_wifi_power);

int __init golfu_wifi_init(void)
{
	int ret = 0;

	printk(KERN_INFO "[WLAN] %s\n", __func__);

	ath6kl_prealloc_mem();

	ret= gpio_request(GOLFU_GPIO_WIFI_IRQ, "WLAN_IRQ");
	if (ret) {
		printk(KERN_INFO "[WLAN] %s: gpio_request is failed!!\n", __func__);
		return ret;
	}
	ret = gpio_direction_output(GOLFU_GPIO_WIFI_IRQ, 0);
	if (ret) {
		printk(KERN_INFO "[WLAN] %s: gpio_direction_output is failed\n", __func__);
	}
	gpio_free(GOLFU_GPIO_WIFI_IRQ);

	return ret;
}
