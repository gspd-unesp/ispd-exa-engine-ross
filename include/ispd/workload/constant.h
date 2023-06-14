#ifndef WORKLOAD_CONSTANT_H
#define WORKLOAD_CONSTANT_H

#include <ispd/workload/workload.h>
#include <ispd/customer/task.h>
#include <ispd/log.h>
#include <ross.h>

struct workload_constant {
	double wl_proc_size;
	double wl_comm_size;
	unsigned long amount;
	interarrival_dist_type dist_type;
};

void workload_constant_next(struct workload_constant *wl, struct task *t, tw_stime *offset)
{
	t->proc_size = wl->wl_proc_size;
	t->comm_size = wl->wl_comm_size;

	switch(wl->dist_type) {
		case FIXED:
			*offset = 0;
			break;
		default:
			ispd_error("Unknown interarrival distribution type %u.", wl->dist_type);
	}
}

#endif // WORKLOAD_CONSTANT_H
