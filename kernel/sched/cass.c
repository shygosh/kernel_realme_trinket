// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023-2024 Sultan Alsawaf <sultan@kerneltoast.com>.
 */

/**
 * DOC: Capacity Aware Superset Scheduler (CASS) description
 *
 * The Capacity Aware Superset Scheduler (CASS) optimizes runqueue selection of
 * CFS tasks. By using CPU capacity as a basis for comparing the relative
 * utilization between different CPUs, CASS fairly balances load across CPUs of
 * varying capacities. This results in improved multi-core performance,
 * especially when CPUs are overutilized because CASS doesn't clip a CPU's
 * utilization when it eclipses the CPU's capacity.
 *
 * As a superset of capacity aware scheduling, CASS implements a hierarchy of
 * criteria to determine the better CPU to wake a task upon between CPUs that
 * have the same relative utilization. This way, single-core performance,
 * latency, and cache affinity are all optimized where possible.
 *
 * CASS doesn't feature explicit energy awareness but its basic load balancing
 * principle results in decreased overall energy, often better than what is
 * possible with explicit energy awareness. By fairly balancing load based on
 * relative utilization, all CPUs are kept at their lowest P-state necessary to
 * satisfy the overall load at any given moment.
 */

#include <linux/version.h>

#ifndef WF_TTWU
#define WF_TTWU (0x1000)
#endif

#define cass_fits_cpu(cap, max) ((cap) * 1138 < (max) * 1024)

struct cass_cpu_cand {
	int cpu;
	unsigned long cap;
	unsigned long util;
};

static int cass_select_task_rq(struct task_struct *p, int prev_cpu, int sd_flag,
			       int wake_flags, int sibling_count_hint, bool rt)
{
	struct cass_cpu_cand cands[2], *best = cands;
	int cidx = 0, cpu, p_cpu, p_queued, adj, perf;
	bool overload_fallback = true;
	unsigned long p_util;
	cpumask_t cpus;

	/*
	 * If there aren't any valid CPUs which are active, then just return the
	 * first valid CPU since it's possible for certain types of tasks to run
	 * on inactive CPUs.
	 */
	if (unlikely(!cpumask_and(&cpus, cpu_active_mask, p->cpus_ptr)))
		return cpumask_first(p->cpus_ptr);

	/*
	 * We would like to narrow down valid CPUs to either low-power or
	 * performance CPUs for optimized task placement. We'll rely on Android
	 * custom implementation of oom_score_adj for the determining factor:
	 * FOREGROUND_APP_ADJ <= perf <= PERCEPTIBLE_APP_ADJ
	 */
	adj = p->signal->oom_score_adj;
	perf = adj >= 0 && adj <= 224;
	if (unlikely(!cpumask_and(&cpus, &cpus, perf ? cpu_perf_mask : cpu_lp_mask)))
		cpumask_and(&cpus, cpu_active_mask, p->cpus_ptr);

	if (!rt) {
		if ((wake_flags & WF_TTWU) || (sd_flag & SD_BALANCE_WAKE)) {
			record_wakee(p);

			if (sched_feat(WA_IDLE)) {
				int this_cpu = smp_processor_id();
				bool sync = (wake_flags & WF_SYNC) &&
					!(current->flags & PF_EXITING) &&
					this_cpu != prev_cpu &&
					!wake_wide(p, sibling_count_hint) &&
					cpumask_test_cpu(this_cpu, &cpus);
				int wa_cpu = wake_affine_idle(this_cpu, prev_cpu, sync);
				if (wa_cpu != nr_cpumask_bits)
					return wa_cpu;
			}
		}

		p_cpu = task_cpu(p);
		p_queued = task_on_rq_queued(p) || current == p;

		/*
		 * We need task's util to find best candidate,
		 * sync it up to prev_cpu's last_update_time.
		 */
		if (!(wake_flags & WF_FORK) && !(sd_flag & SD_BALANCE_FORK))
			sync_entity_load_avg(&p->se);

#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 20, 0))
		p_util = _task_util_est(p);
#else
		p_util = _task_util_est(p) | UTIL_AVG_UNCHANGED;
#endif
	}

	/*
	 * Find the best CPU to wake @p on.
	 *
	 * Note: @curr->cpu must be initialized before this loop ends. This is
	 * necessary to ensure @best->cpu contains a valid CPU upon returning;
	 * otherwise, if only one CPU is allowed and it is skipped before
	 * @curr->cpu is set, then @best->cpu will be garbage.
	 */
retry:
	for_each_cpu(cpu, &cpus) {
		/* Use the free candidate slot for @curr */
		struct cass_cpu_cand *curr = &cands[cidx];
		struct rq *rq = cpu_rq(cpu);

		/* Get this CPU's capacity and utilization */
		curr->cpu = cpu;
		curr->util = READ_ONCE(rq->cfs.avg.util_est.enqueued);
		curr->cap = capacity_orig_of(curr->cpu);
		curr->cap -= min_t(unsigned long, cpu_util_rt(rq) + cpu_util_dl(rq) +
				   cpu_util_irq(rq), curr->cap - 1);

		/*
		 * Add @p's utilization to this CPU if it's not @p's CPU, to
		 * find what this CPU's relative utilization would look like
		 * if @p were on it.
		 */
		if (!rt && p_queued && curr->cpu != p_cpu)
			curr->util += p_util;
		else if (!rt && !p_queued)
			curr->util += p_util;

		/* Calculate the relative utilization for this CPU candidate */
		curr->util = curr->util * SCHED_CAPACITY_SCALE / curr->cap;

		/*
		 * Check if this CPU is better than the best CPU found so far.
		 * If @best == @curr then there's no need to compare them, but
		 * cidx still needs to be changed to the other candidate slot.
		 */
		if (best == curr || curr->util <= best->util) {
			best = curr;
			cidx ^= 1;
		}
	}

	if (overload_fallback &&
	    !cass_fits_cpu(best->util * best->cap / SCHED_CAPACITY_SCALE, best->cap)) {
		overload_fallback = false;
		cpumask_and(&cpus, cpu_active_mask, p->cpus_ptr);
		cpumask_and(&cpus, &cpus, perf ? cpu_lp_mask : cpu_perf_mask);
		if (!cpumask_empty(&cpus))
			goto retry;
	}

	return best->cpu;
}

static int cass_select_task_rq_fair(struct task_struct *p, int prev_cpu,
				    int sd_flag, int wake_flags, int sibling_count_hint)
{
	return cass_select_task_rq(p, prev_cpu, sd_flag, wake_flags, sibling_count_hint, false);
}

int cass_select_task_rq_rt(struct task_struct *p, int prev_cpu, int sd_flag,
			   int wake_flags, int sibling_count_hint)
{
	return cass_select_task_rq(p, prev_cpu, sd_flag, wake_flags, sibling_count_hint, true);
}
