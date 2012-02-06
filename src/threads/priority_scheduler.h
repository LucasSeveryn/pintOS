#ifndef __LIB_KERNEL_PRIORITY_SCHEDULER_H
#define __LIB_KERNEL_PRIORITY_SCHEDULER_H

#include "threads/thread.h"
#include "lib/kernel/list.h"

struct priority_scheduler{
	struct list lists[PRI_MAX + 1];
	int max_priority;
	int size;
};

bool ps_empty( struct priority_scheduler * );

void ps_init( struct priority_scheduler * );

/* Priority Queue insertion */
void ps_push( struct priority_scheduler *, struct thread * );

/* Highest priority */ 
struct thread * ps_pop( struct priority_scheduler * );
struct thread * ps_pull( struct priority_scheduler * );

void ps_update( struct priority_scheduler *, struct thread * );

#endif