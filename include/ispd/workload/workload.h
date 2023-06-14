#ifndef WORKLOAD_H
#define WORKLOAD_H

#include <ispd/customer/task.h>
#include <ross.h>

typedef enum workload_gen_type {
	CONSTANT,
} workload_gen_type;

typedef enum interarrival_dist_type {
	FIXED,
	EXPONENTIAL,
	POISSON,
} interarrival_dist_type;

typedef struct workload {
	workload_gen_type wl_type;
	interarrival_dist_type ia_dist_type;

	double wl_proc_size;
	double wl_comm_size;
	unsigned long amount;
} workload;

extern void workload_next(workload *wl, task *t, tw_stime *offset);

#endif // WORKLOAD_H
