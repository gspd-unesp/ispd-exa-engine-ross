#include <ispd/services/machine.h>
#include <ispd/builder/builder.h>
#include <ispd/message.h>
#include <ispd/log.h>
#include <ross.h>

static inline double time_to_proc(const machine_state *s, const double proc_size)
{
	return proc_size / ((1.0 - s->load) * s->power);
}

void machine_init(machine_state *s, tw_lp *lp)
{
	// Initialize the machine state, fetching it from the model built.
	memcpy(s, g_model[lp->gid].state, sizeof(*s));

	ispd_log(LOG_DEBUG, "Machine [LP GID: %lu] has been initialized [P: %.2lf, C: %u, L: %.2lf].", lp->gid,
	    s->power, s->cores, s->load);
}

void machine_event(machine_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
{
	double delay;
	tw_event *e;
	ispd_message *m;

	delay = time_to_proc(s, msg->t.proc_size);

	s->proc_mflops += msg->t.proc_size;
	s->proc_time += delay;
	s->proc_tasks++;

	e = tw_event_new(msg->t.origin, tw_rand_exponential(lp->rng, delay / 5.0), lp);
	m = tw_event_data(e);

	m->t = msg->t;
	m->t.state = PROCESSED;

	tw_event_send(e);
}

void machine_rc_event(machine_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
{
	double delay;

	delay = time_to_proc(s, msg->t.proc_size);

	s->proc_mflops -= msg->t.proc_size;
	s->proc_time -= delay;
	s->proc_tasks--;
}

void machine_final(machine_state *s, tw_lp *lp)
{
	ispd_log(LOG_DEBUG,
	    "\nMachine @ LP %lu\n"
	    " - Processed MFLOPS: %lf MFLOPS.\n"
	    " - Processed Time..: %lf seconds.\n"
	    " - Processed Tasks.: %u tasks.\n",
	    lp->gid, s->proc_mflops, s->proc_time, s->proc_tasks);
}
