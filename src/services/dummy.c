#include <ispd/services/dummy.h>
#include <ispd/workload/workload.h>
#include <ispd/log.h>
#include <ross.h>

void dummy_init(dummy_state *s, tw_lp *lp)
{
	ispd_log(LOG_DEBUG, "Dummy [LP GID: %lu, NODE: %lu] has been initialized.", lp->gid, g_tw_mynode);
}


void other_dummy_init(dummy_state *s, tw_lp *lp)
{
	ispd_log(LOG_DEBUG, "Dummy [LP GID: %lu, NODE: %lu] has been finalized.", lp->gid, g_tw_mynode);
}

void dummy_event(dummy_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {}

void dummy_rc_event(dummy_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {}

void dummy_final(dummy_state *s, tw_lp *lp) {}
