// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Sultan Alsawaf <sultan@kerneltoast.com>.
 */

#ifdef CONFIG_SCHED_CASS
extern int cass_select_task_rq_rt(struct task_struct *p, int prev_cpu, int sd_flag,
			   int wake_flags, int sibling_count);

#define select_task_rq_rt cass_select_task_rq_rt
#define select_task_rq_dl cass_select_task_rq_rt
#endif /* CONFIG_SCHED_CASS */
