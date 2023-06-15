#include <ispd/services/link.h>
#include <ispd/message.h>
#include <ispd/builder/builder.h>
#include <ispd/core.h>
#include <ispd/log.h>
#include <ross.h>

ENGINE_INLINE static double time_to_comm(const link_state *s, const double comm_size)
{
	return comm_size / ((1.0 - s->load) * s->bandwidth) + s->latency;
}

void link_init(link_state *s, tw_lp *lp)
{
	// Initialize the machine state, fetching it from the model built.
	memset(s, 0, sizeof(*s));
	memcpy(s, g_model[lp->gid].state, sizeof(*s));

	ispd_log(LOG_DEBUG, "Link [LP GID: %lu] has been initialized [B: %.2lf, LT: %.2lf, L: %.2lf].", s->bandwidth,
	    s->latency, s->load);
}

void link_event(link_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
{
	double delay;
	tw_stime now;
	tw_stime departure_time;
	tw_stime waiting_time;
	tw_stime last_free_time;
	tw_event *e;
	ispd_message *m;

	now = tw_now(lp);

	// This is the time needed for the packet to be completely transmitted
	// over this network link that is connecting two ends.
	delay = time_to_comm(s, msg->t.comm_size);

	waiting_time = ROSS_MAX(0, s->next_free_time - now);
	departure_time = now + waiting_time + delay;

	// Update the next free time instant. This indicates the next instant
	// of time in which this link can start transmitting another packet.
	last_free_time = s->next_free_time;
	s->next_free_time = departure_time;

	// Update the link statistics.
	s->comm_mbits += msg->t.comm_size;
	s->comm_time += delay;
	s->comm_packets++;

	e = tw_event_new(s->to, departure_time - now, lp);
	m = tw_event_data(e);
	m->t = msg->t;
	m->t.link_free_time = last_free_time;

	tw_event_send(e);

	ispd_log(LOG_DEBUG, "Task (%.2lf, %.2lf) [LINK: %lu, A: %.2lf, W: %.2lf, D: %.2lf].", m->t.proc_size,
	    m->t.comm_size, lp->gid, now, waiting_time, departure_time);
}

void link_rc_event(link_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
{
	double delay;

	delay = time_to_comm(s, msg->t.comm_size);

	s->next_free_time = msg->t.link_free_time;

	s->comm_mbits -= msg->t.comm_size;
	s->comm_time -= delay;
	s->comm_packets--;

	ispd_log(LOG_DEBUG, "[RB-LINK]: Task (%.2lf, %.2lf) [LINK: %lu, A: %.2lf, W: %.2lf, D: %.2lf].", lp->gid,
	    msg->t.proc_size, msg->t.comm_size, lp->gid, tw_now(lp), -1.0, -1.0);
}

void link_final(link_state *s, tw_lp *lp)
{
	ispd_log(LOG_INFO,
	    "\nLink @ LP %lu\n"
	    " - Last Activity Time..: %lf seconds.\n"
	    " - Communicated Mbits..: %lf Mbits.\n"
	    " - Communicated Time...: %lf seconds.\n"
	    " - Communicated Packets: %u packets.\n",
	    lp->gid, s->next_free_time, s->comm_mbits, s->comm_time, s->comm_packets);
}
