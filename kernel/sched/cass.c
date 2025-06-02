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

#define lsub_positive(_ptr, _val) do {			\
	typeof(_ptr) ptr = (_ptr);			\
	*ptr -= min_t(typeof(*ptr), *ptr, _val);	\
} while (0)

struct cass_cpu_cand {
	int cpu;
	unsigned long cap;
	unsigned long load;
};

static int cass_select_task_rq_fair(struct task_struct *p, int prev_cpu, int sd_flag,
				    int wake_flags, int sibling_count_hint)
{
	struct cass_cpu_cand cands[2], *best = cands;
	bool p_queued = task_on_rq_queued(p) || current == p;
	int cidx = 0, cpu, adj, perf;
	unsigned long p_load = 0;
	struct cpumask cpus;

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

	/*
	 * We need task's load to find best candidate,
	 * sync it up to prev_cpu's last_update_time.
	 */
	if (!(wake_flags & WF_FORK) && !(sd_flag & SD_BALANCE_FORK)) {
		sync_entity_load_avg(&p->se);
		p_load = task_h_load(p);
	}

	/*
	 * Find the best CPU to wake @p on.
	 *
	 * Note: @curr->cpu must be initialized before this loop ends. This is
	 * necessary to ensure @best->cpu contains a valid CPU upon returning;
	 * otherwise, if only one CPU is allowed and it is skipped before
	 * @curr->cpu is set, then @best->cpu will be garbage.
	 */
	for_each_cpu(cpu, &cpus) {
		/* Use the free candidate slot for @curr */
		struct cass_cpu_cand *curr = &cands[cidx];
		struct rq *rq = cpu_rq(cpu);

		/* Get this CPU's capacity and load */
		curr->cpu = cpu;
		curr->load = rq->cfs.avg.load_avg;
		curr->cap = capacity_orig_of(curr->cpu);
		lsub_positive(&curr->cap, cpu_util_rt(rq) +
			      cpu_util_dl(rq) + cpu_util_irq(rq));
		if (!curr->cap)
			curr->cap = 1;

		/*
		 * Add @p's load to this CPU if it's not @p's CPU, to
		 * find what this CPU's relative load would look like
		 * if @p were on it.
		 */
		if (p_queued && curr->cpu != prev_cpu)
			curr->load += p_load;
		else if (!p_queued)
			curr->load += p_load;

		/* Calculate the relative load for this CPU candidate */
		curr->load = curr->load * SCHED_CAPACITY_SCALE / curr->cap;

		/*
		 * Check if this CPU is better than the best CPU found so far.
		 * If @best == @curr then there's no need to compare them, but
		 * cidx still needs to be changed to the other candidate slot.
		 */
		if (best == curr || curr->load <= best->load) {
			best = curr;
			cidx ^= 1;
		}
	}

	return best->cpu;
}
