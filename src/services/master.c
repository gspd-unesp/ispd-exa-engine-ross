#include "ispd/scheduler/scheduler.h"
#include <ispd/services/master.h>
#include <ispd/workload/workload.h>
#include <ispd/builder/builder.h>
#include <ispd/routing_table.h>
#include <ispd/scheduler/round_robin.h>
#include <ispd/scheduler/workqueue.h>
#include <ispd/log.h>
#include <string.h>
#include <ross.h>

extern routing_table *g_routing_table;

ENGINE_INLINE static void master_sched_init(master_state *s)
{
	switch(s->scheduler_type) {
		case SCHED_ROUND_ROBIN:
			sched_rr_init((sched_round_robin **)&s->scheduler, s);
			break;
		case SCHED_WORKQUEUE:
			sched_workqueue_init((sched_workqueue **)&s->scheduler, s);
			break;
		default:
			ispd_error("Unknown master scheduler type (%u).", s->scheduler_type);
	}
}

ENGINE_INLINE void master_sched_firstround(master_state *s, tw_lp *lp)
{
	int i = 0;
	switch(s->scheduler_type) {
		// In the case of a master using a round-robin scheduling algorithm,
		// it is possible that all tasks are generated at the first round and
		// already send to the processing resources that are will process them.
		case SCHED_ROUND_ROBIN:
			sched_rr_firstround(s->scheduler, s, lp);
			break;
		case SCHED_WORKQUEUE:
			sched_workqueue_firstround(s->scheduler, s, lp);
			break;
		default:
			ispd_error("Unknown master scheduler type (%u).", s->scheduler_type);
	}
}

void master_init(master_state *s, tw_lp *lp)
{
	int i;

	// Initialize the master state, fetching it from the model built.
	memcpy(s, g_model[lp->gid].state, sizeof(*s));

	master_sched_init(s);
	master_sched_firstround(s, lp);

	ispd_log(LOG_DEBUG, "Master [LP GID: %lu] has been initialized [SC: %u].\n", lp->gid, s->slave_count);
}

void master_event(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
{
	tw_event *e;
	ispd_message *m;
	tw_stime offset;
	route *r;

	bf->c0 = 0;
	bf->c1 = 0;

	if(msg->t.state == PROCESSED) {
		bf->c0 = 1;
		s->completed_tasks++;

		// Checks if the remaining tasks to be generated is equals to 0.
		// Therefore, there is no more tasks to be generated and, then no
		// task will be sent.
		if(unlikely(s->wl.amount == 0))
			return;

		bf->c1 = 1;

		switch(s->scheduler_type) {
			case SCHED_ROUND_ROBIN:
				// Nothing is done here, since all tasks has already
				// been scheduled and, therefore, there is no more
				// task to be schedule.
				break;
			case SCHED_WORKQUEUE:
				sched_workqueue_event(s, bf, msg, lp);
				break;
			default:
				ispd_error("Unknown master scheduler type (%u).", s->scheduler_type);
		}
	}
}

void master_rc_event(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
{
	if((bf->c0 == 1) && msg->t.state == PROCESSED) {
		bf->c0 = 0;
		s->completed_tasks--;

		if(bf->c1 == 1) {
			bf->c1 = 0;
			switch(s->scheduler_type) {
				case SCHED_ROUND_ROBIN:
					break;
				case SCHED_WORKQUEUE:
					sched_workqueue_rc_event(s, bf, msg, lp);
					break;
				default:
					ispd_error("Unknown master scheduler type (%u).", s->scheduler_type);
			}
		}
	}

	ispd_log(LOG_DEBUG, "[RB-MASTER]: Master (%.2lf, %.2lf) [MASTER: %lu, CT: %u].", msg->t.proc_size,
	    msg->t.comm_size, lp->gid, tw_now(lp), s->completed_tasks);
}

void master_final(master_state *s, tw_lp *lp)
{
	ispd_log(LOG_INFO,
	    "\nMaster @ LP %lu\n"
	    " - Completed Tasks: %u tasks.\n",
	    lp->gid, s->completed_tasks);
}
