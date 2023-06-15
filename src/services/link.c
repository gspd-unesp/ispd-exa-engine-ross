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
	tw_stime departure_time;
	tw_stime waiting_time;
	tw_event *e;
	ispd_message *m;

	bf->c0 = 0;

	// This is the time needed for the packet to be completely transmitted
	// over this network link that is connecting two ends.
	delay = time_to_comm(s, msg->t.comm_size);

	waiting_time = ROSS_MAX(0, s->next_free_time - tw_now(lp));

	// Checks if the task has arrived before the link is available. Therefore,
	// the task will have to wait to be transmitted, incurring in a non-zero
	// waiting time. Altough, it is used ROSS_MAX as in
	//
	//   waiting_time = ROSS_MAX(0, s->next_free_time - tw_now(lp))
	//
	// to obtain the waiting time. It is necessary, to set the bitfield that
	// identify that this task has waited, without doing that the reverse
	// computation will not know whther the task has waited.
	if(tw_now(lp) < s->next_free_time)
		bf->c0 = 1;

	departure_time = tw_now(lp) + waiting_time + delay;

	// Update the next free time instant. This indicates the next instant
	// of time in which this link can start transmitting another packet.
	s->next_free_time = departure_time;

	// Update the link statistics.
	s->comm_mbits += msg->t.comm_size;
	s->comm_time += delay;
	s->comm_packets++;

	e = tw_event_new(s->to, departure_time - tw_now(lp), lp);
	m = tw_event_data(e);
	m->t = msg->t;

	tw_event_send(e);

	ispd_log(LOG_DEBUG, "Task (%.2lf, %.2lf) [LINK: %lu, A: %.2lf, W: %.2lf, D: %.2lf].", lp->gid, m->t.proc_size,
	    m->t.comm_size, tw_now(lp), waiting_time, departure_time);
}

void link_rc_event(link_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
{
	double delay;

	delay = time_to_comm(s, msg->t.comm_size);

	// @Todo: Implement the reverse computation of next free time.

	s->comm_mbits -= msg->t.comm_size;
	s->comm_time -= delay;
	s->comm_packets--;

	ispd_log(LOG_DEBUG, "[RB-LINK]: Task (%.2lf, %.2lf) [LINK: %lu, A: %.2lf, W: %.2lf, D: %.2lf].", lp->gid,
	    msg->t.proc_size, msg->t.comm_size, lp->gid, -1.0, -1.0);
}

void link_final(link_state *s, tw_lp *lp)
{
	ispd_log(LOG_DEBUG,
	    "\nLink @ LP %lu\n"
	    " - Last Activity Time..: %lf seconds.\n"
	    " - Communicated Mbits..: %lf Mbits.\n"
	    " - Communicated Time...: %lf seconds.\n"
	    " - Communicated Packets: %u packets.\n",
	    lp->gid, s->next_free_time, s->comm_mbits, s->comm_time, s->comm_packets);
}
