#ifndef ISPD_SCHEDULER_WORKQUEUE_H
#define ISPD_SCHEDULER_WORKQUEUE_H

#include <ispd/services/master.h>
#include <ispd/core.h>
#include <ispd/log.h>

typedef struct sched_workqueue {
} sched_workqueue;

ENGINE_INLINE static void sched_workqueue_init(sched_workqueue **wq, master_state *s)
{
	// Checks if the Workqueue scheduler could not be allocated.
	if(!((*wq) = malloc(sizeof(sched_workqueue))))
		ispd_error("Workqueue scheduler could not be allocated.");
}

#endif // ISPD_SCHEDULER_WORKQUEUE_H
