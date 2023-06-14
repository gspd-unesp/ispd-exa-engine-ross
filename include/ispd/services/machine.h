#ifndef SERVICE_MACHINE_H
#define SERVICE_MACHINE_H

#include <ispd/message.h>
#include <ross.h>

typedef struct machine_state {
	double power;
	double load;
	unsigned cores;
	double proc_mflops;
	double proc_time;
	unsigned proc_tasks;
} machine_state;

extern void machine_init(machine_state *s, tw_lp *lp);
extern void machine_event(machine_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp);
extern void machine_rc_event(machine_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp);
extern void machine_final(machine_state *s, tw_lp *lp);

#endif // SERVICE_MACHINE_H
