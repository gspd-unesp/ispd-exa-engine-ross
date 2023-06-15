#ifndef ISPD_SCHEDULER_ROUND_ROBIN_H
#define ISPD_SCHEDULER_ROUND_ROBIN_H

#include <ispd/services/master.h>
#include <ispd/log.h>
#include <ispd/core.h>
#include <ross.h>

typedef struct sched_round_robin {
	/// \brief A pointer to the master in which this scheduled
	///	   is being used as scheduling algorithm.
	master_state *master;

	/// \brief The next slave identifier that will be selected.
	tw_lpid next_slave_index;
} sched_round_robin;

ENGINE_INLINE void sched_rr_init(sched_round_robin **rr, master_state *s)
{
	// Checks if the Round-Robin scheduler structure could not be allocated.
	if(!((*rr) = malloc(sizeof(sched_round_robin))))
		ispd_error("Round-robin scheduler could not be allocated.");
	(*rr)->master = s;
	(*rr)->next_slave_index = 0;
}

ENGINE_INLINE void sched_rr_next(sched_round_robin *rr, tw_lpid *id)
{
	if(unlikely(rr->next_slave_index == rr->master->slave_count))
		rr->next_slave_index = 0;
	*id = rr->master->slaves[rr->next_slave_index++];
#ifdef SCHEDULER_ROUND_ROBIN_DEBUG
	ispd_log(LOG_DEBUG, "Master ? selected %lu using round-robin.", *id);
#endif // SCHEDULER_ROUND_ROBIN_DEBUG
}

#endif // ISPD_SCHEDULER_ROUND_ROBIN_H
