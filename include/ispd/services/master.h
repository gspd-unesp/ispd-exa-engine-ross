#ifndef SERVICE_MASTER_H
#define SERVICE_MASTER_H

#include <ross.h>
#include <ispd/message.h>
#include <ispd/workload/workload.h>
#include <ispd/scheduler/scheduler.h>

typedef struct master_state {
	/// \brief A count of how many tasks have been processed.
	///	   by the master's slaves.
	unsigned completed_tasks;

	/// \brief An array containing the logical processes
	/// 	   global identifiers that represent the services
	/// 	   that are this master's slaves.
	tw_lpid *slaves;

	/// \brief The count of how many slaves this master has.
	unsigned slave_count;

	void *scheduler;
	scheduler_type scheduler_type;

	workload wl;
} master_state;

extern void master_init(master_state *s, tw_lp *lp);
extern void master_event(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp);
extern void master_rc_event(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp);
extern void master_final(master_state *s, tw_lp *lp);

#endif // SERVICE_MASTER_H
