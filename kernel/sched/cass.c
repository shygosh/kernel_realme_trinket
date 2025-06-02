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

#ifndef WF_TTWU
#define WF_TTWU (0x8000)
#endif

#define lsub_positive(_ptr, _val) do {			\
	typeof(_ptr) ptr = (_ptr);			\
	*ptr -= min_t(typeof(*ptr), *ptr, _val);	\
} while (0)

struct cass_cpu_cand {
	int cpu;
	unsigned long load;
};

static int cass_select_task_rq_fair(struct task_struct *p, int prev_cpu, int sd_flag,
				    int wake_flags, int sibling_count_hint)
{
	struct cass_cpu_cand cands[2], *best = cands;
	int sync = (wake_flags & WF_SYNC) && !(current->flags & PF_EXITING);
	int p_queued = task_on_rq_queued(p) || current == p, want_affine = 0;
	int this_cpu = smp_processor_id(), cidx = 0, cpu, adj;
	unsigned long p_load = 0;
	const struct cpumask *asym_cpus;
	struct cpumask cpus;

	/*
	 * Narrow down valid CPUs to either low-power or performance CPUs for optimized
	 * task placement. We'll rely on Android's custom implementation of oom_score_adj
	 * to determine which group of CPUs should we place @p on.
	 */
	adj = p->signal->oom_score_adj;
	asym_cpus = (adj > -1 && adj < 225) ? cpu_perf_mask : cpu_lp_mask;
	if (unlikely(!cpumask_and(&cpus, cpu_online_mask, asym_cpus)))
		return prev_cpu;

	/*
	 * Stock wake_affine idle path.
	 */
	if ((sd_flag & SD_BALANCE_WAKE) || (wake_flags & WF_TTWU)) {
		int wa_cpu = nr_cpumask_bits;

		record_wakee(p);

		want_affine = !wake_wide(p, sibling_count_hint) &&
			cpumask_test_cpu(cpu, &cpus);

		if (want_affine)
			wa_cpu = wake_affine_idle(this_cpu, prev_cpu, sync);

		if (wa_cpu != nr_cpumask_bits)
			return select_idle_sibling(p, prev_cpu, wa_cpu);
	}

	/*
	 * We need task's load to find best candidate,
	 * sync it up to prev_cpu's last_update_time.
	 */
	if (!(sd_flag & SD_BALANCE_FORK) && !(wake_flags & WF_FORK)) {
		sync_entity_load_avg(&p->se);
		p_load = task_h_load(p);
	}

	/*
	 * Invalidate sync wake if @p prefers wake wide.
	 */
	if (sync)
		sync = sync && want_affine;

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

		/* Get this CPU's load */
		curr->cpu = cpu;
		curr->load = rq->cfs.avg.load_avg;

		/*
		 * Add @p's load to this CPU if it's not @p's CPU, to
		 * find what this CPU's relative load would look like
		 * if @p were on it.
		 */
		if (sync) {
			if (curr->cpu != prev_cpu)
				curr->load += p_load;
			if (curr->cpu == this_cpu)
				lsub_positive(&curr->load, task_h_load(current));
		} else {
			if (p_queued && curr->cpu != prev_cpu)
				curr->load += p_load;
			if (!p_queued)
				curr->load += p_load;
		}

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
