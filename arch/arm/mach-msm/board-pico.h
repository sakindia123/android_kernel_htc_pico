/* linux/arch/arm/mach-msm/board-pico.h
 * Copyright (C) 2009 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#ifndef __ARCH_ARM_MACH_MSM_BOARD_PICO_H
#define __ARCH_ARM_MACH_MSM_BOARD_PICO_H

#include <mach/board.h>

#define MSM_MEM_BASE		0x10000000
#ifdef CONFIG_SMART_DUMMY_PICO
#define MSM_MEM_SIZE		0x10000000
#else
#define MSM_MEM_SIZE		0x20000000
#endif

#define MSM_LINUX_BASE_OFFSET	0x02C00000

#ifdef CONFIG_MSM_RESERVE_PMEM
#define MSM_MM_HEAP_SIZE	0x00300000
#else
/* MSM_PMEM_MDP_SIZE + MSM_PMEM_ADSP_SIZE + MSM_RAM_CONSOLE_SIZE + MSM_HTC_DEBUG_INFO_SIZE + MSM_FB_SIZE */
#define MSM_MM_HEAP_SIZE	0x02700000
#endif

#define MSM_LINUX_BASE          (MSM_MEM_BASE + MSM_LINUX_BASE_OFFSET) /* 2MB alignment */
#define MSM_LINUX_SIZE          (MSM_MEM_SIZE - MSM_LINUX_BASE_OFFSET - MSM_MM_HEAP_SIZE)

#ifndef CONFIG_MSM_RESERVE_PMEM
#define MSM_PMEM_MDP_BASE       (MSM_MEM_BASE + MSM_MEM_SIZE - MSM_MM_HEAP_SIZE) /* 0x2CF00000 */
#define MSM_PMEM_MDP_SIZE       0x01400000

#define MSM_PMEM_ADSP_BASE      (MSM_PMEM_MDP_BASE + MSM_PMEM_MDP_SIZE) /* 0x2ED00000 */
#define MSM_PMEM_ADSP_SIZE      0x01000000

#define MSM_RAM_CONSOLE_BASE    (MSM_PMEM_ADSP_BASE + MSM_PMEM_ADSP_SIZE) /* MSM_HTC_RAM_CONSOLE_PHYS must be the same */
#else
#define MSM_RAM_CONSOLE_BASE    (MSM_MEM_BASE + MSM_MEM_SIZE - MSM_MM_HEAP_SIZE) /* MSM_HTC_RAM_CONSOLE_PHYS must be the same */
#endif

#define MSM_RAM_CONSOLE_SIZE    MSM_HTC_RAM_CONSOLE_SIZE

#define MSM_FB_BASE             (MSM_RAM_CONSOLE_BASE + MSM_RAM_CONSOLE_SIZE + MSM_HTC_DEBUG_INFO_SIZE) /* 0x2FE00000 */
#define MSM_FB_SIZE             0x00200000

#define PICO_GPIO_TO_INT(x)           (x+64) /* from gpio_to_irq */

#define PICO_GPIO_USB_ID_PIN          (19)
#define PICO_POWER_KEY                (20)
#define PICO_GPIO_PS_HOLD         (25)
#define PICO_GPIO_WIFI_IRQ            (29)
////#define PICO_GPIO_RESET_BTN_N         (36)
#define PICO_GPIO_SDMC_CD_N           (38)
//#define PICO_GPIO_UP_INT_N            (39)
#define PICO_GPIO_CHG_INT		(40)
#define PICO_GPIO_VOL_DOWN_XB         (48)
#define PICO_GPIO_VOL_DOWN            (49)

/* Camera I2C */
#define PICO_GPIO_CAMERA_SCL		  (60)
#define PICO_GPIO_CAMERA_SDA		  (61)

/* WLAN SD data */
#define PICO_GPIO_SD_D3               (64)
#define PICO_GPIO_SD_D2               (65)
#define PICO_GPIO_SD_D1               (66)
#define PICO_GPIO_SD_D0               (67)
#define PICO_GPIO_SD_CMD              (63)
#define PICO_GPIO_SD_CLK_0            (62)

/* I2C */
#define PICO_GPIO_I2C_SCL             (131)
#define PICO_GPIO_I2C_SDA             (132)

/* MicroSD */
#define PICO_GPIO_SDMC_CLK_1          (56)
#define PICO_GPIO_SDMC_CMD            (55)
#define PICO_GPIO_SDMC_D3             (51)
#define PICO_GPIO_SDMC_D2             (52)
#define PICO_GPIO_SDMC_D1             (53)
#define PICO_GPIO_SDMC_D0             (54)

/* BT PCM */
#define PICO_GPIO_AUD_PCM_DO          (68)
#define PICO_GPIO_AUD_PCM_DI          (69)
#define PICO_GPIO_AUD_PCM_SYNC        (70)
#define PICO_GPIO_AUD_PCM_CLK         (71)

//#define PICO_GPIO_UP_RESET_N          (76)
//#define PICO_GPIO_FLASHLIGHT          (85)
#define PICO_GPIO_UART3_RX            (86)
#define PICO_GPIO_UART3_TX            (87)
#define PICO_GPIO_VOL_UP              (92)
//#define PICO_GPIO_LS_EN               (93)
#define PICO_GPIO_WIFI_SHUTDOWN_N       (108)
//#define PICO_GPIO_V_USBPHY_3V3_EN     (109)
//#define PICO_GPIO_VIBRATOR_ON         (116)

/* Compass  */
#define PICO_GPIO_GSENSORS_INT         (39)
#define PICO_LAYOUTS			{ \
		{ {  0, -1, 0}, { -1,  0, 0}, {0, 0, -1} }, \
		{ {  0, -1, 0}, { -1,  0, 0}, {0, 0,  1} }, \
		{ { -1,  0, 0}, {  0,  1, 0}, {0, 0, -1} }, \
		{ {  1,  0, 0}, {  0,  0, 1}, {0, 1,  0} }  \
					}

/* Proximity Sensor */
#define PICO_GPIO_PROXIMITY_INT       (36)

/* BT */
#define PICO_GPIO_BT_UART1_RTS        (43)
#define PICO_GPIO_BT_UART1_CTS        (44)
#define PICO_GPIO_BT_UART1_RX         (45)
#define PICO_GPIO_BT_UART1_TX         (46)
#define PICO_GPIO_BT_RESET_N          (90)
#define PICO_GPIO_BT_HOST_WAKE        (112)
#define PICO_GPIO_BT_CHIP_WAKE        (122)
#define PICO_GPIO_BT_SD_N             (123)

/* Touch Panel */
#define PICO_GPIO_TP_ATT_N            (18)
#define PICO_V_TP_3V3_EN              (31)
#define PICO_LCD_RSTz		        (118)
#define PICO_GPIO_TP_RST_N            (120)

/* Accessory */
#define PICO_GPIO_AUD_HP_DET		(21)
#define PICO_GPIO_AUD_REMO_PRES		(114)

/*Camera AF VCM POWER*/
#define PICO_GPIO_VCM_PD              (126)
#define PICO_GPIO_CAM_RST_N           (125)

/*Display*/
#define PICO_GPIO_LCD_ID0             (34)
#define PICO_GPIO_LCD_ID1             (35)
#define PICO_GPIO_MDDI_TE             (97)
#define PICO_GPIO_LCD_RST_N           (118)
#define PICO_GPIO_LCM_1v8_EN             (5)
#define PICO_GPIO_LCM_2v85_EN             (6)

int __init pico_init_keypad(void);
int pico_init_mmc(unsigned int sys_rev);
int __init pico_init_panel(void);
int __init pico_wifi_init(void);
#endif /* GUARD */

