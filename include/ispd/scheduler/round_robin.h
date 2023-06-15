#ifndef ISPD_SCHEDULER_ROUND_ROBIN_H
#define ISPD_SCHEDULER_ROUND_ROBIN_H

#include <ispd/services/master.h>
#include <ispd/routing_table.h>
#include <ispd/log.h>
#include <ispd/core.h>
#include <ross.h>

extern routing_table *g_routing_table;

typedef struct sched_round_robin {
	/// \brief A pointer to the master in which this scheduled
	///	   is being used as scheduling algorithm.
	master_state *master;

	/// \brief The next slave identifier that will be selected.
	tw_lpid next_slave_index;
} sched_round_robin;

ENGINE_INLINE void sched_rr_init(sched_round_robin **rr, master_state *s)
{
	// Checks if the Round-Robin scheduler structure could not be allocated.
	if(!((*rr) = malloc(sizeof(sched_round_robin))))
		ispd_error("Round-robin scheduler could not be allocated.");
	(*rr)->master = s;
	(*rr)->next_slave_index = 0;
}

ENGINE_INLINE void sched_rr_next(sched_round_robin *rr, tw_lpid *id)
{
	if(unlikely(rr->next_slave_index == rr->master->slave_count))
		rr->next_slave_index = 0;
	*id = rr->master->slaves[rr->next_slave_index++];
#ifdef SCHEDULER_ROUND_ROBIN_DEBUG
	ispd_log(LOG_DEBUG, "Master ? selected %lu using round-robin.", *id);
#endif // SCHEDULER_ROUND_ROBIN_DEBUG
}

ENGINE_INLINE void sched_rr_firstround(sched_round_robin *rr, master_state *s, tw_lp *lp)
{
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

	while(s->wl.amount) {
		sched_rr_next(rr, &next_slave_id);
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
}

#endif // ISPD_SCHEDULER_ROUND_ROBIN_H
