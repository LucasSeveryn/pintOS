#ifndef __LIB_KERNEL_PRIORITY_SCHEDULER_H
#define __LIB_KERNEL_PRIORITY_SCHEDULER_H

#include "threads/thread.h"
#include "lib/kernel/list.h"

struct priority_scheduler{
	struct list lists[PRI_MAX + 1];		/* 64 queues of threads */
	int max_priority;					/* Highest priority of a thread in the scheduler */
	int size;							/* Number of threads currently inside */
	int sleeping;						/* Number of sleeping threads in it */
};

bool ps_empty( struct priority_scheduler * );

void ps_init( struct priority_scheduler * );

/* Priority Queue insertion */
void ps_push( struct priority_scheduler *, struct thread * );

/* Highest priority */ 
struct thread * ps_pop( struct priority_scheduler * );
struct thread * ps_pull( struct priority_scheduler * );

bool ps_contains( struct priority_scheduler *, struct thread * );

void ps_update_auto( struct thread * );
void ps_update( struct priority_scheduler *, struct thread * );

#endif