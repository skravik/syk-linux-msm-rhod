/* linux/arch/arm/mach-msm/board-htctopaz.h
 */
#ifndef __ARCH_ARM_MACH_MSM_BOARD_HTCTOPAZ_H
#define __ARCH_ARM_MACH_MSM_BOARD_HTCTOPAZ_H

#include <mach/board.h>


#define DECLARE_MSM_IOMAP
#include <mach/msm_iomap.h>


#define TOPA100_CAM_DAT0	0
#define TOPA100_CAM_DAT1	1
#define TOPA100_CAM_DAT2	2
#define TOPA100_CAM_DAT3	3
#define TOPA100_CAM_DAT4	4
#define TOPA100_CAM_DAT5	5
#define TOPA100_CAM_DAT6	6
#define TOPA100_CAM_DAT7	7
#define TOPA100_CAM_DAT8	8
#define TOPA100_CAM_DAT9	9
#define TOPA100_CAM_DAT10	10
#define TOPA100_CAM_DAT11	11
#define TOPA100_CAM_PCLK	12
#define TOPA100_CAM_HSYNC_IN	13
#define TOPA100_CAM_VSYNC_N	14
#define TOPA100_CAM_MCLK	15

#define TOPA100_CAM_PWR1	30

#define TOPA100_CAM_VCMPDP  91
#define TOPA100_CAM1_RST	92
#define TOPA100_CAM2_RST	93

#define TOPA100_BAT_IN				28

#define TOPA100_USB_AC_PWR			32
#define TOPA100_CHARGE_EN_N			44

#define TOPA100_VOLDOWN_KEY			24
#define TOPA100_VOLUP_KEY			25
#define TOPA100_POWER_KEY			83

#define TOPA100_UART2DM_RTS			19	/* BT */
#define TOPA100_UART2DM_CTS			20	/* BT */
#define TOPA100_UART2DM_RX			21	/* BT */

#define TOPA100_CABLE_IN1			18
#define TOPA100_CABLE_IN2			45
#define TOPA100_H2W_DATA			31
#define TOPA100_H2W_CLK				46
#define TOPA100_AUD_HSMIC_DET_N		17
#define TOPA100_USBPHY_RST			100

#define TOPA100_LCD_VSYNC			97

#define TOPA100_CAMERA_SWITCH		30
#define TOPA100_CAMERA_VCMPDP		91
#define TOPA100_CAMERA1_RST			92
#define TOPA100_CAMERA2_RST			93
#define TOPA100_CAMERA1_PWR			107
#define TOPA100_CAMERA2_PWR			109


#endif 
