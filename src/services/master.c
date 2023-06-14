#include <ispd/services/master.h>
#include <ispd/workload/workload.h>
#include <ispd/builder/builder.h>
#include <ispd/routing_table.h>
#include <ispd/log.h>
#include <string.h>
#include <ross.h>

extern routing_table *g_routing_table;

void master_init(master_state *s, tw_lp *lp)
{
	// Initialize the master state, fetching it from the model built.
	memcpy(s, g_model[lp->gid].state, sizeof(*s));

	// @Test: This should be removed later.
	int i, task_amount;
	tw_event *e;
	ispd_message *m;
	route *r;
	tw_lpid next_slave_index;

	task_amount = 200000;
	next_slave_index = 0;

	for(i = 0; i < task_amount; i++) {
		if(next_slave_index == s->slave_count)
			next_slave_index = 0;
		routing_table_get(g_routing_table, lp->gid, s->slaves[next_slave_index], &r);
		next_slave_index++;

		e = tw_event_new(r->route[0], tw_rand_exponential(lp->rng, 5.0), lp);
		m = tw_event_data(e);

		m->t.proc_size = 150.0;
		m->t.comm_size = 200.0;
		m->t.origin = lp->gid;
		m->t.state = JUST_GENERATED;

		tw_event_send(e);
	}
	// @Test: End Test.

	ispd_log(LOG_DEBUG, "Master [LP GID: %lu] has been initialized [SC: %u].\n", s->slave_count);
}

void master_event(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
{
	bf->c0 = 0;

	if(msg->t.state == PROCESSED) {
		bf->c0 = 1;
		s->completed_tasks++;
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
