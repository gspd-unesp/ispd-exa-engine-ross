#ifndef ISPD_SCHEDULER_WORKQUEUE_H
#define ISPD_SCHEDULER_WORKQUEUE_H

#include <ispd/services/master.h>
#include <ispd/routing_table.h>
#include <ispd/core.h>
#include <ispd/log.h>

extern routing_table *g_routing_table;

typedef struct sched_workqueue {
} sched_workqueue;

ENGINE_INLINE static void sched_workqueue_init(sched_workqueue **wq, master_state *s)
{
	// Checks if the Workqueue scheduler could not be allocated.
	if(!((*wq) = malloc(sizeof(sched_workqueue))))
		ispd_error("Workqueue scheduler could not be allocated.");
}

ENGINE_INLINE static void sched_workqueue_firstround(sched_workqueue *wq, master_state *s, tw_lp *lp)
{
	int i;
	tw_stime now;
	tw_event *e;
	ispd_message *m;
	route *r;

	now = tw_now(lp);

	// This represents the offset with relation to the current
	// logical process local virtual time, that represents the
	// time in which the task will arrive at the outgoing link.
	tw_stime offset;

	for(i = 0; i < s->slave_count; i++) {
		// Checks if the remaining tasks to be generated is equals to 0.
		// Therefore, there is no more tasks to be generated and, then no
		// task will be sent.
		if(unlikely(s->wl.amount == 0))
			return;
		routing_table_get(g_routing_table, lp->gid, s->slaves[i], &r);

		workload_next_time(&s->wl, &offset, lp->rng);

		e = tw_event_new(r->route[0], offset - now, lp);
		m = tw_event_data(e);

		workload_next_task(&s->wl, &m->t);

		m->t.origin = lp->gid;
		m->t.destination = s->slaves[i];
		m->t.state = JUST_GENERATED;

		tw_event_send(e);
	}
}

ENGINE_INLINE static void sched_workqueue_event(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
{
	tw_lpid dest;
	tw_stime offset;
	tw_event *e;
	ispd_message *m;
	route *r;

	dest = msg->t.destination;

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

ENGINE_INLINE static void sched_workqueue_rc_event(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
{
	s->wl.amount++;
}

#endif // ISPD_SCHEDULER_WORKQUEUE_H
