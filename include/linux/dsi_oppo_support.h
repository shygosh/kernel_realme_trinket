/***************************************************************
** Copyright (C),  2018,  OPPO Mobile Comm Corp.,  Ltd
** CONFIG_VENDOR_EDIT
** File : dsi_oppo_support.h
** Description : display driver private management
** Version : 1.0
** Date : 2018/03/17
** Author : Jie.Hu@PSW.MM.Display.Stability
**
** ------------------------------- Revision History: -----------
**  <author>        <data>        <version >        <desc>
**   Hu.Jie          2018/03/17        1.0           Build this moudle
******************************************************************/
#ifndef _DSI_OPPO_SUPPORT_H_
#define _DSI_OPPO_SUPPORT_H_

#include <linux/err.h>
#include <linux/string.h>
#include <linux/notifier.h>

/* A hardware display blank change occurred */
#define OPPO_DISPLAY_EVENT_BLANK			0x01

/* A hardware display blank early change occurred */
#define OPPO_DISPLAY_EARLY_EVENT_BLANK		0x02

enum oppo_display_support_list {
	OPPO_SAMSUNG_S6E3FC2X01_DISPLAY_FHD_DSC_CMD_PANEL = 0,
	OPPO_SAMSUNG_S6E8FC1X01_DISPALY_FHD_PLUS_VIDEO_PANEL = 1,
	OPPO_BOE_HX83112A_FHD_PLUS_VIDEO_PANEL=2,
	OPPO_HUAXING_HX83112A_DISPALY_FHD_PLUS_VIDEO_PANEL=3,
	OPPO_HLT_HX83112A_DISPALY_FHD_PLUS_VIDEO_PANEL=4,
	OPPO_TM_NT36672A_DISPALY_FHD_PLUS_VIDEO_PANEL=5,
//#ifdef CONFIG_ODM_WT_EDIT
//Hongzhu.Su@ODM_WT.MM.Display.Lcd., Start 2020/03/09, add CABC cmd used for power saving
	TRULY_AUO_ILI9881H_DISPALY_HDP_VIDEO_PANEL=6,
	HLT_BOE_NT36525B_DISPALY_HDP_VIDEO_PANEL=7,
	INNOLUX_INX_ILI9881H_DISPALY_HDP_VIDEO_PANEL,
//Hongzhu.Su@ODM_WT.MM.Display.Lcd., End 2020/03/09, add CABC cmd used for power saving
//#endif /* CONFIG_ODM_WT_EDIT */
	OPPO_DISPLAY_UNKNOW,
};

enum oppo_display_power_status {
	OPPO_DISPLAY_POWER_OFF = 0,
	OPPO_DISPLAY_POWER_DOZE,
	OPPO_DISPLAY_POWER_ON,
	OPPO_DISPLAY_POWER_DOZE_SUSPEND,
	OPPO_DISPLAY_POWER_ON_UNKNOW,
};

enum oppo_display_feature {
	OPPO_DISPLAY_HDR = 0,
	OPPO_DISPLAY_SEED,
	OPPO_DISPLAY_HBM,
	OPPO_DISPLAY_LBR,
	OPPO_DISPLAY_ULPS,
	OPPO_DISPLAY_ESD_CHECK,
	OPPO_DISPLAY_DYNAMIC_MIPI,
	OPPO_DISPLAY_PARTIAL_UPDATE,
	OPPO_DISPLAY_FEATURE_MAX,
};

typedef struct panel_serial_info
{
	int reg_index;
	uint64_t year;
	uint64_t month;
	uint64_t day;
	uint64_t hour;
	uint64_t minute;
	uint64_t second;
	uint64_t reserved[2];
} PANEL_SERIAL_INFO;


typedef struct oppo_display_notifier_event {
	enum oppo_display_power_status status;
	void *data;
}OPPO_DISPLAY_NOTIFIER_EVENT;

bool is_silence_reboot(void);

int set_oppo_display_vendor(const char * display_name);

void set_oppo_display_power_status(enum oppo_display_power_status power_status);

enum oppo_display_power_status get_oppo_display_power_status(void);

#endif /* _DSI_OPPO_SUPPORT_H_ */

