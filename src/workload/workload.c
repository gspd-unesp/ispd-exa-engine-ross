#include <ispd/workload/workload.h>
#include <ispd/log.h>

void workload_next_time(workload *wl, tw_stime *offset, tw_rng_stream *rng)
{
	switch(wl->ia_dist_type) {
		case FIXED_INTERARRIVAL:
			*offset = 0;
			break;
		case EXPONENTIAL_INTERARRIVAL:
			*offset = tw_rand_exponential(rng, 5.0);
			break;
		default:
			ispd_error("Unknown worload interarrival distribution type (%u).", wl->ia_dist_type);
	}
}

void workload_next_task(workload *wl, task *t)
{
	switch(wl->wl_type) {
		case CONSTANT_WORKLOAD:
			t->proc_size = wl->wl_proc_size;
			t->comm_size = wl->wl_comm_size;
			break;
		default:
			ispd_error("Unknown workload generation type (%u).", wl->wl_type);
	}

	wl->amount--;
}
