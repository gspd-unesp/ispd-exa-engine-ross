#include "ispd/workload/workload.h"
#include <stdio.h>
#include <stdlib.h>
#include <ispd/routing_table.h>
#include <ispd/services/master.h>
#include <ispd/services/link.h>
#include <ispd/services/machine.h>
#include <ispd/services/dummy.h>
#include <ispd/builder/builder.h>
#include <ispd/routing_table.h>
#include <ispd/log.h>
#include <ross.h>
#include <ross-extern.h>

tw_peid mapping(tw_lpid gid)
{
	return (tw_peid)gid / g_tw_nlp;
}

tw_lptype lps_type[] = {
    [MASTER] =
	{
	    (init_f)master_init,
	    (pre_run_f)NULL,
	    (event_f)master_event,
	    (revent_f)master_rc_event,
	    (commit_f)NULL,
	    (final_f)master_final,
	    (map_f)mapping,
	    sizeof(master_state),
	},
    [LINK] =
	{
	    (init_f)link_init,
	    (pre_run_f)NULL,
	    (event_f)link_event,
	    (revent_f)link_rc_event,
	    (commit_f)NULL,
	    (final_f)link_final,
	    (map_f)mapping,
	    sizeof(link_state),
	},
    [MACHINE] =
	{
	    (init_f)machine_init,
	    (pre_run_f)NULL,
	    (event_f)machine_event,
	    (revent_f)machine_rc_event,
	    (commit_f)NULL,
	    (final_f)machine_final,
	    (map_f)mapping,
	    sizeof(machine_state),
	},
    [DUMMY] =
	{
	    (init_f)dummy_init,
	    (pre_run_f)NULL,
	    (event_f)dummy_event,
	    (revent_f)dummy_rc_event,
	    (commit_f)NULL,
	    (final_f)dummy_final,
	    (map_f)mapping,
	    sizeof(dummy_state),
	},
    [OTHER_DUMMY] =
	{
	    (init_f)other_dummy_init,
	    (pre_run_f)NULL,
	    (event_f)dummy_event,
	    (revent_f)dummy_rc_event,
	    (commit_f)NULL,
	    (final_f)dummy_final,
	    (map_f)mapping,
	    sizeof(dummy_state),
	},
    {0},
};
const tw_optdef opt[] = {
    TWOPT_GROUP("iSPD Model"),
    TWOPT_END(),
};

routing_table *g_routing_table;

int main(int argc, char **argv)
{
	g_tw_lookahead = 1e-6;

	int i;
	int nlp_per_pe;

	logfile_set(NULL);
	routing_table_load(&g_routing_table, "routes.route");

	tw_opt_add(opt);
	tw_init(&argc, &argv);

	printf("Node Count: %u.\n", tw_nnodes());

	model_set_nlp(17);

	tw_lpid *slaves = malloc(sizeof(tw_lpid) * 8);
	for(i = 0; i < 8; i++)
		slaves[i] = 2 * i + 2;
	model_lp_settype(0, MASTER,
	    &(struct master_state){.completed_tasks = 0,
		.slaves = slaves,
		.slave_count = 8,
		.scheduler_type = SCHED_WORKQUEUE,
		.wl =
		    {
			.wl_type = CONSTANT_WORKLOAD,
			.ia_dist_type = EXPONENTIAL_INTERARRIVAL,
			.wl_proc_size = 150.0,
			.wl_comm_size = 200.0,
			.amount = 20,
		    }},
	    sizeof(master_state));

	for(i = 1; i < 17; i += 2) {
		model_lp_settype(i, LINK,
		    &(link_state){.from = 0,
			.to = i + 1,
			.bandwidth = 5.0,
			.load = 0.0,
			.latency = 7.0,
			.comm_mbits = 0,
			.comm_time = 0,
			.comm_packets = 0},
		    sizeof(link_state));
		model_lp_settype(i + 1, MACHINE,
		    &(machine_state){.power = 500.0, .load = 0.0, .proc_mflops = 0, .proc_time = 0, .proc_tasks = 0},
		    sizeof(machine_state));
	}

	nlp_per_pe = (int)ceil((double)g_model_nlp / tw_nnodes());

	tw_define_lps(nlp_per_pe, sizeof(ispd_message));
	for(i = 0; i < nlp_per_pe; i++) {
		tw_lp *lp = g_tw_lp[i];

		if(lp->gid >= g_model_nlp)
			tw_lp_settype(i, &lps_type[DUMMY]);
		else {
			lp_model_state *lp_state = &g_model[lp->gid];
			tw_lp_settype(i, &lps_type[lp_state->type]);
		}
	}

	tw_run();
	tw_end();
	return 0;
}
