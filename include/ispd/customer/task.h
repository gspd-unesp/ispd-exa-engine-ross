#ifndef CUSTOMER_TASK_H
#define CUSTOMER_TASK_H

#include <ross.h>

typedef enum task_state { JUST_GENERATED, PROCESSED } task_state;

typedef struct task {
	double proc_size;
	double comm_size;
	tw_lpid origin;
	task_state state;
} task;

#endif // CUSTOMER_TASK_H
