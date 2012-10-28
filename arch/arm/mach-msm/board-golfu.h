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

#ifndef __ARCH_ARM_MACH_MSM_BOARD_GOLFU_H
#define __ARCH_ARM_MACH_MSM_BOARD_GOLFU_H

#include <mach/board.h>
#include "board-bahama.h"

int __init golfu_init_keypad(void);
int __init golfu_init_panel(void);
int __init golfu_wifi_init(void);

#define MSM_MEM_BASE		0x10000000
#define MSM_MEM_SIZE		0x20000000

#define MSM_LINUX_BASE_OFFSET	0x03000000

/* MSM_MM_HEAP_SIZE
 * = MSM_FB_SIZE
 * + (MSM_HTC_DEBUG_INFO_SIZE + MSM_RAM_CONSOLE_SIZE)
 * + MSM_PMEM_MDP_SIZE
 * + MSM_PMEM_ADSP_SIZE
 */
#define MSM_FB_BASE             0x2FE00000
#define MSM_FB_SIZE             0x00200000

#define MSM_LINUX_BASE          (MSM_MEM_BASE + MSM_LINUX_BASE_OFFSET) /* 2MB alignment */
#define MSM_LINUX_SIZE          (MSM_MEM_SIZE - MSM_LINUX_BASE_OFFSET - MSM_FB_SIZE - 0x100000)/*1MB for ram console*/

/* Must be same as MSM_HTC_RAM_CONSOLE_PHYS */
#define MSM_RAM_CONSOLE_BASE    0x2FD00000
#define MSM_RAM_CONSOLE_SIZE    MSM_HTC_RAM_CONSOLE_SIZE

#define MSM_PMEM_MDP_SIZE       0x01400000
#define MSM_PMEM_ADSP_SIZE      0x00D00000

#define GOLFU__GPIO_TO_INT(x)       (x+64) /* from gpio_to_irq */

#define GOLFU_GPIO_USB_ID         	   (17)
#define GOLFU_POWER_KEY            	   (41)
#define GOLFU_GPIO_PS_HOLD         	   (25)
#define GOLFU_GPIO_WIFI_IRQ            (94)
#define GOLFU_GPIO_SDMC_CD_N           (38)
#define GOLFU_GPIO_CHG_INT			   (40)
#define GOLFU_GPIO_VOL_DOWN            (48)
#define GOLFU_GPIO_VOL_UP              (112)

/* Camera */
#define GOLFU_GPIO_CAMERA_SCL		   (60)
#define GOLFU_GPIO_CAMERA_SDA		   (61)
#define GOLFU_GPIO_CAMERA_RESET        (125)
#define GOLFU_GPIO_CAMERA_STANDBY      (126)
#define GOLFU_GPIO_CAMERA_MCLK         (15)
#define GOLFU_GPIO_CAM_D1V8_EN		   (11) /* LDO trigger D1V8 */
#define GOLFU_GPIO_CAM_A2V85_EN		   (12) /* LDO trigger A2V85 */
#define GOLFU_GPIO_CAM_ID              (57) /* From XB board and later */


/* WLAN SD data */
#define GOLFU_GPIO_SD_D3               (64)
#define GOLFU_GPIO_SD_D2               (65)
#define GOLFU_GPIO_SD_D1               (66)
#define GOLFU_GPIO_SD_D0               (67)
#define GOLFU_GPIO_SD_CMD              (63)
#define GOLFU_GPIO_SD_CLK_0            (62)

/* I2C */
#define GOLFU_GPIO_I2C_SCL             (131)
#define GOLFU_GPIO_I2C_SDA             (132)

/* MicroSD */
#define GOLFU_GPIO_SDMC_CLK_1          (56)
#define GOLFU_GPIO_SDMC_CMD            (55)
#define GOLFU_GPIO_SDMC_D3             (51)
#define GOLFU_GPIO_SDMC_D2             (52)
#define GOLFU_GPIO_SDMC_D1             (53)
#define GOLFU_GPIO_SDMC_D0             (54)

/* BT PCM */
#define GOLFU_GPIO_AUD_PCM_DO          (68)
#define GOLFU_GPIO_AUD_PCM_DI          (69)
#define GOLFU_GPIO_AUD_PCM_SYNC        (70)
#define GOLFU_GPIO_AUD_PCM_CLK         (71)

#define GOLFU_GPIO_UART3_RX            (86)
#define GOLFU_GPIO_UART3_TX            (87)

#define GOLFU_GPIO_WIFI_SHUTDOWN_N     (127)

/* Compass  */
#define GOLFU_GPIO_GSENSORS_INT        (124)
#define GOLFU_LAYOUTS			{ \
		{ {  0, -1, 0}, { -1,  0, 0}, {0, 0, -1} }, \
		{ {  0, -1, 0}, { -1,  0, 0}, {0, 0,  1} }, \
		{ { -1,  0, 0}, {  0,  1, 0}, {0, 0, -1} }, \
		{ {  1,  0, 0}, {  0,  0, 1}, {0, 1,  0} }  \
					}

/* Proximity Sensor */
#define GOLFU_GPIO_PROXIMITY_INT       (36)

/* BT */
#define GOLFU_GPIO_BT_UART1_RTS        (43)
#define GOLFU_GPIO_BT_UART1_CTS        (44)
#define GOLFU_GPIO_BT_UART1_RX         (45)
#define GOLFU_GPIO_BT_UART1_TX         (46)


/* Touch Panel */
#define GOLFU_GPIO_TP_ATT_N            (18)
#define GOLFU_V_TP_3V3_EN              (31)
#define GOLFU_LCD_RSTz		           (118)
#define GOLFU_GPIO_TP_RST_N            (120)

/* Accessory */
#define GOLFU_GPIO_AUD_HP_DET		   (37)
#define GOLFU_GPIO_AUD_REMO_PRES	   (114)

/*Camera AF VCM POWER*/
#define GOLFU_GPIO_VCM_PD              (126)
#define GOLFU_GPIO_CAM_RST_N           (125)

/*Display*/
#define GOLFU_GPIO_LCD_ID0             (34)
#define GOLFU_GPIO_LCD_ID1             (35)
#define GOLFU_GPIO_MDDI_TE             (97)
#define GOLFU_GPIO_LCD_RST_N           (118)
#define GOLFU_GPIO_LCM_1v8_EN          (5)
#define GOLFU_GPIO_LCM_2v85_EN         (6)

/* NFC */
#define GOLFU_GPIO_NFC_VEN	(8)
#define GOLFU_GPIO_NFC_INT		(27)
#define GOLFU_GPIO_NFC_DL	(7)

#endif /* GUARD */

