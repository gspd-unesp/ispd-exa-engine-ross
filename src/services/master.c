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

ENGINE_INLINE static void master_sched_next(master_state *s, tw_lpid *id)
{
	switch(s->scheduler_type) {
		case SCHED_ROUND_ROBIN:
			sched_rr_next(s->scheduler, id);
			break;
		default:
			ispd_error("Unknown master scheduler type (%u).", s->scheduler_type);
	}
}

ENGINE_INLINE static void master_sched_and_send(master_state *s, tw_lp *lp)
{
	// Checks if the remaining tasks to be generated is equals to 0.
	// Therefore, there is no more tasks to be generated and, then no
	// task will be sent.
	if(unlikely(s->wl.amount == 0))
		return;

	tw_event *e;
	ispd_message *m;
	route *r;

	// This represents the offset with relation to the current
	// logical process local virtual time, that represents the
	// time in which the task will arrive at the outgoing link.
	tw_stime offset;

	// This contains the logical process' identifier of the
	// computing resource that will receive the scheduled task.
	// This identifier will be selected by  scheduling algorithm
	// policy.
	tw_lpid next_slave_id;

	master_sched_next(s, &next_slave_id);
	routing_table_get(g_routing_table, lp->gid, next_slave_id, &r);

	workload_next_time(&s->wl, &offset, lp->rng);

	e = tw_event_new(r->route[0], offset, lp);
	m = tw_event_data(e);

	workload_next_task(&s->wl, &m->t);

	m->t.origin = lp->gid;
	m->t.destination = next_slave_id;
	m->t.state = JUST_GENERATED;

	tw_event_send(e);
}

ENGINE_INLINE static void master_wq_sched_and_send(master_state *s, tw_lp *lp, tw_lpid dest)
{
	// Checks if the remaining tasks to be generated is equals to 0.
	// Therefore, there is no more tasks to be generated and, then no
	// task will be sent.
	if(unlikely(s->wl.amount == 0))
		return;

	tw_event *e;
	ispd_message *m;
	route *r;

	// This represents the offset with relation to the current
	// logical process local virtual time, that represents the
	// time in which the task will arrive at the outgoing link.
	tw_stime offset;

	routing_table_get(g_routing_table, lp->gid, dest, &r);

	workload_next_time(&s->wl, &offset, lp->rng);

	e = tw_event_new(r->route[0], offset, lp);
	m = tw_event_data(e);

	workload_next_task(&s->wl, &m->t);

	m->t.origin = lp->gid;
	m->t.destination = dest;
	m->t.state = JUST_GENERATED;

	tw_event_send(e);
}


ENGINE_INLINE void master_sched_firstround(master_state *s, tw_lp *lp)
{
	int i = 0;
	switch(s->scheduler_type) {
		// In the case of a master using a round-robin scheduling algorithm,
		// it is possible that all tasks are generated at the first round and
		// already send to the processing resources that are will process them.
		case SCHED_ROUND_ROBIN:
			while(s->wl.amount)
				master_sched_and_send(s, lp);
			break;
		case SCHED_WORKQUEUE:
			for(i = 0; i < s->slave_count; i++)
				master_wq_sched_and_send(s, lp, s->slaves[i]);
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

		if(unlikely(s->wl.amount == 0))
			return;

		bf->c1 = 1;

		switch(s->scheduler_type) {
			case SCHED_ROUND_ROBIN:
				// Schedules the next task and send it to the selected
				// computing resource.
				master_sched_and_send(s, lp);
				break;
			case SCHED_WORKQUEUE:
				routing_table_get(g_routing_table, lp->gid, msg->t.destination, &r);

				workload_next_time(&s->wl, &offset, lp->rng);

				e = tw_event_new(r->route[0], offset, lp);
				m = tw_event_data(e);

				workload_next_task(&s->wl, &m->t);

				m->t.origin = lp->gid;
				m->t.destination = msg->t.destination;
				m->t.state = JUST_GENERATED;

				tw_event_send(e);
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
	}
}

void master_final(master_state *s, tw_lp *lp)
{
	ispd_log(LOG_DEBUG,
	    "\nMaster @ LP %lu\n"
	    " - Completed Tasks: %u tasks.\n",
	    lp->gid, s->completed_tasks);
}
