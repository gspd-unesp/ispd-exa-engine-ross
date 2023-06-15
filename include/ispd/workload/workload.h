#ifndef WORKLOAD_H
#define WORKLOAD_H

#include <ispd/customer/task.h>
#include <ross.h>

typedef enum workload_gen_type {
	CONSTANT_WORKLOAD,
} workload_gen_type;

typedef enum interarrival_dist_type {
	FIXED_INTERARRIVAL,
	EXPONENTIAL_INTERARRIVAL,
	POISSON_INTERARRIVAL,
} interarrival_dist_type;

typedef struct workload {
	workload_gen_type wl_type;
	interarrival_dist_type ia_dist_type;

	double wl_proc_size;
	double wl_comm_size;
	unsigned amount;
} workload;

extern void workload_next_time(workload *wl, tw_stime *offset, tw_rng_stream *rng);
extern void workload_next_task(workload *wl, task *t);

#endif // WORKLOAD_H
