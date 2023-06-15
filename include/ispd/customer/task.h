#ifndef CUSTOMER_TASK_H
#define CUSTOMER_TASK_H

#include <ross.h>

typedef enum task_state { JUST_GENERATED, PROCESSED } task_state;

typedef struct task {
	double proc_size;
	double comm_size;
	tw_lpid origin;
	tw_lpid destination;
	task_state state;
	tw_stime link_free_time;
} task;

#endif // CUSTOMER_TASK_H
