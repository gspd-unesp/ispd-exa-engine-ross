#include <ispd/workload/workload.h>
#include <ispd/log.h>

void workload_next(workload *wl, task *t, tw_stime *offset)
{
	switch(wl->wl_type) {
		case CONSTANT:
			t->proc_size = wl->wl_proc_size;
			t->comm_size = wl->wl_comm_size;
			break;
		default:
			ispd_error("Unknown workload generation type %u.", wl->wl_type);
	}

	switch(wl->ia_dist_type) {
		case FIXED:
			*offset = 0;
			break;
		default:
			ispd_error("Unknown interarrival distribution type %u.", wl->ia_dist_type);
	}
}
