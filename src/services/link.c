#include <ispd/services/link.h>
#include <ispd/message.h>
#include <ispd/builder/builder.h>
#include <ispd/log.h>
#include <ross.h>

static inline double time_to_comm(const link_state *s, const double comm_size)
{
	return comm_size / ((1.0 - s->load) * s->bandwidth) + s->latency;
}

void link_init(link_state *s, tw_lp *lp)
{
	// Initialize the machine state, fetching it from the model built.
	memcpy(s, g_model[lp->gid].state, sizeof(*s));

	ispd_log(LOG_DEBUG, "Link [LP GID: %lu] has been initialized [B: %.2lf, LT: %.2lf, L: %.2lf].", s->bandwidth,
	    s->latency, s->load);
}

void link_event(link_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
{
	double delay;
	tw_event *e;
	ispd_message *m;

	delay = time_to_comm(s, msg->t.comm_size);

	s->comm_mbits += msg->t.comm_size;
	s->comm_time += delay;
	s->comm_packets++;

	e = tw_event_new(s->to, delay, lp);
	m = tw_event_data(e);
	m->t = msg->t;

	tw_event_send(e);
}

void link_rc_event(link_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
{
	double delay;

	delay = time_to_comm(s, msg->t.comm_size);

	s->comm_mbits -= msg->t.comm_size;
	s->comm_time -= delay;
	s->comm_packets--;
}

void link_final(link_state *s, tw_lp *lp)
{
	ispd_log(LOG_DEBUG,
	    "\nLink @ LP %lu\n"
	    " - Communicated Mbits..: %lf Mbits.\n"
	    " - Communicated Time...: %lf seconds.\n"
	    " - Communicated Packets: %u packets.\n",
	    lp->gid, s->comm_mbits, s->comm_time, s->comm_packets);
}
