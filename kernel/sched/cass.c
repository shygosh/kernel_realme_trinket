// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023-2024 Sultan Alsawaf <sultan@kerneltoast.com>.
 */

struct cass_cpu_cand {
	int cpu;
	unsigned long load;
};

static int cass_select_task_rq_fair(struct task_struct *p, int prev_cpu, int sd_flag,
				    int wake_flags, int sibling_count_hint)
{
	int p_queued = task_on_rq_queued(p) || current == p;
	int cpu, adj = p->signal->oom_score_adj;
	unsigned long p_load = task_h_load(p);
	const struct cpumask *asym_cpus;
	struct cass_cpu_cand best, cand;

	asym_cpus = (adj > -1 && adj < 300) ? cpu_perf_mask : cpu_lp_mask;
	best.cpu = cpumask_any(cpu_active_mask);
	best.load = ULONG_MAX;

	for_each_cpu_and(cpu, cpu_active_mask, asym_cpus) {
		cand.cpu = cpu;
		cand.load = cpu_rq(cpu)->cfs.avg.load_avg;

		if (p_queued && cpu != prev_cpu)
			cand.load += p_load;
		else
			cand.load += p_load;

		if (cand.load < best.load)
			best = cand;
	}

	return best.cpu;
}
