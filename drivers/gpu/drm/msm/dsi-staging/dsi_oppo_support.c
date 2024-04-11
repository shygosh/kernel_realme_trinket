/***************************************************************
** Copyright (C),  2018,  OPPO Mobile Comm Corp.,  Ltd
** CONFIG_VENDOR_EDIT
** File : dsi_oppo_support.c
** Description : display driver private management
** Version : 1.0
** Date : 2018/03/17
** Author : Jie.Hu@PSW.MM.Display.Stability
**
** ------------------------------- Revision History: -----------
**  <author>        <data>        <version >        <desc>
**   Hu.Jie          2018/03/17        1.0           Build this moudle
******************************************************************/
#include <linux/dsi_oppo_support.h>
#include <soc/oppo/boot_mode.h>
#include <soc/oppo/oppo_project.h>
#include <soc/oppo/device_info.h>
#include <soc/oppo/boot_mode.h>
#include <linux/notifier.h>
#include <linux/module.h>

static enum oppo_display_support_list  oppo_display_vendor = OPPO_DISPLAY_UNKNOW;
static enum oppo_display_power_status oppo_display_status = OPPO_DISPLAY_POWER_OFF;

static BLOCKING_NOTIFIER_HEAD(oppo_display_notifier_list);

bool is_silence_reboot(void) {
	if((MSM_BOOT_MODE__SILENCE == get_boot_mode()) || (MSM_BOOT_MODE__SAU == get_boot_mode())) {
		return true;
	} else {
		return false;
	}
}

int set_oppo_display_vendor(const char * display_name) {
	if (display_name == NULL)
		return -1;

/*W9001377@MM.Display.Driver.Stability. 20191207 */
/*HQ001218@MM.Display.Driver.Stability. 20191224 */
/* add lcd info begin */
	if (!strcmp(display_name,"dsi_samsung_fhd_plus_dsc_cmd_display")) {
		oppo_display_vendor = OPPO_SAMSUNG_S6E3FC2X01_DISPLAY_FHD_DSC_CMD_PANEL;
		register_device_proc("lcd", "S6E3FC2X01", "samsung1024");
	} else if (!strcmp(display_name,"dsi_s6e8fc1x01_samsumg_vid_display")) {
		oppo_display_vendor = OPPO_SAMSUNG_S6E8FC1X01_DISPALY_FHD_PLUS_VIDEO_PANEL;
		register_device_proc("lcd", "S6E8FC1X01", "samsung1024");
	} else if (!strcmp(display_name,"dsi_hx83112a_boe_vid_display")) {
		oppo_display_vendor = OPPO_BOE_HX83112A_FHD_PLUS_VIDEO_PANEL;
		register_device_proc("lcd", "HX83112A", "boe1024");
	} else if (!strcmp(display_name,"dsi_hx83112a_huaxing_vid_display")) {
		oppo_display_vendor = OPPO_HUAXING_HX83112A_DISPALY_FHD_PLUS_VIDEO_PANEL;
		register_device_proc("lcd", "HX83112A", "huaxing1024");
    } else if (!strcmp(display_name,"dsi_hx83112a_hlt_vid_display")) {
		oppo_display_vendor = OPPO_HLT_HX83112A_DISPALY_FHD_PLUS_VIDEO_PANEL;
		register_device_proc("lcd", "HX83112A", "hlt1024");
	 } else if (!strcmp(display_name,"dsi_nt36672a_tm_vid_display")) {
		oppo_display_vendor = OPPO_TM_NT36672A_DISPALY_FHD_PLUS_VIDEO_PANEL;
		register_device_proc("lcd", "NT36672A", "tm1024");
//#ifdef CONFIG_ODM_WT_EDIT
//Hongzhu.Su@ODM_WT.MM.Display.Lcd., Start 2020/03/09, add CABC cmd used for power saving
	 } else if ((!strcmp(display_name,"dsi_ili9881h_truly_auo_video_display")) || \
			    (!strcmp(display_name, "dsi_ili9881h_truly_auo_gg3_video_display"))) {
		oppo_display_vendor = TRULY_AUO_ILI9881H_DISPALY_HDP_VIDEO_PANEL;
		register_device_proc("lcd", "ILI9881H", "truly auo");
	 } else if ((!strcmp(display_name,"dsi_nt36525b_hlt_boe_video_display")) || \
			    (!strcmp(display_name, "dsi_nt36525b_hlt_boe_gg3_video_display"))) {
		oppo_display_vendor = HLT_BOE_NT36525B_DISPALY_HDP_VIDEO_PANEL;
		register_device_proc("lcd", "NT36525B", "hlt boe");
	 } else if ((!strcmp(display_name,"dsi_ili9881h_innolux_inx_video_display")) || \
			    (!strcmp(display_name, "dsi_ili9881h_innolux_inx_gg3_video_display"))) {
		oppo_display_vendor = INNOLUX_INX_ILI9881H_DISPALY_HDP_VIDEO_PANEL;
		register_device_proc("lcd", "ILI9881H", "innolux inx");
//Hongzhu.Su@ODM_WT.MM.Display.Lcd., End 2020/03/09, add CABC cmd used for power saving
//#endif /* CONFIG_ODM_WT_EDIT */
	} else {
		oppo_display_vendor = OPPO_DISPLAY_UNKNOW;
		pr_err("%s panel vendor info set failed!", __func__);
	}
/* end */
	return 0;
}

void set_oppo_display_power_status(enum oppo_display_power_status power_status) {
	oppo_display_status = power_status;
}

enum oppo_display_power_status get_oppo_display_power_status(void) {
	return oppo_display_status;
}
EXPORT_SYMBOL(get_oppo_display_power_status);
