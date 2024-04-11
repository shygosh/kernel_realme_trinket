/**
 * Copyright 2008-2013 OPPO Mobile Comm Corp., Ltd, All rights reserved.
 * FileName:devinfo.h
 * ModuleName:devinfo
 * Create Date: 2013-10-23
 * Description:add interface to get device information.
*/

#ifndef _DEVICE_INFO_H
#define _DEVICE_INFO_H

#include <linux/list.h>

#define INFO_LEN  24

struct manufacture_info {
	char name[INFO_LEN];
	char *version;
	char *manufacture;
	char *fw_path;
};

struct o_hw_id {
	const char *label;
	const char *match;
	int id;
	struct list_head list;
};

int register_device_proc(char *name, char *version, char *vendor);
/*maxinming_hq added for camera devinfo 20191212*/
int register_device_proc_cam(char *name, char *version);
int register_devinfo(char *name, struct manufacture_info *info);
bool check_id_match(const char *label, const char *id_match, int id);

//#ifdef CONFIG_ODM_WT_EDIT
//Bo.Zhang@ODM_WT.BSP.TP,2020/04/05, added for TP devinfo begain
int register_tp_proc(char *name, char *version, char *manufacture ,char *fw_path);
//Bo.Zhang@ODM_WT.BSP.TP,2020/04/05, added for TP devinfo end
//#endif /* CONFIG_ODM_WT_EDIT */


#endif
