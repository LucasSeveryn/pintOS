#include "threads/priority_scheduler.h"
#include "threads/interrupt.h"
#include <stdio.h>

/* Initializes a scheduler */
void 
ps_init (struct priority_scheduler * ps){
	ps->max_priority = PRI_MIN;
	ps->size = 0;
	ps->sleeping = 0;

	int i;
	for( i = PRI_MAX; i >=  0; i-- ){
		list_init (&ps->lists[i]);
	}
}

bool 
ps_empty (struct priority_scheduler * ps){
	return ps->size == 0;
}

/* Priority Queue insertion */
void 
ps_push (struct priority_scheduler * ps, struct thread * th){

	ps->size++;
	list_push_back (&ps->lists[th->priority], &th->elem);

	if (th->priority > ps->max_priority) {
		ps->max_priority = th->priority;
	}

	th->pss = ps;
}

/* Returns and removes thread with the highest priority */
struct thread * 
ps_pop (struct priority_scheduler * ps){
	ASSERT( !ps_empty (ps) );

	ps->size--;
	struct list_elem * el;
	struct thread * th = NULL;
	int i;
	for( i = ps->max_priority; i >=  0; i-- ){
		if( !list_empty (&ps->lists[i]) ){
			el = list_pop_front (&ps->lists[i]);
			th = list_entry (el, struct thread, elem);

			int j;
			for( j = i; j >=0 ; j-- ){
				if( !list_empty (&ps -> lists[j]) ){
					ps->max_priority = j;
					break;
				}
			}
			break;
		}
	}

	th->pss = NULL;
	return th;
}

/* Returns thread with the highest priority */
struct thread * 
ps_pull (struct priority_scheduler * ps){
	ASSERT( !ps_empty (ps) );

	struct list_elem * el;
	struct thread * th = NULL;
	int i;
	for( i = ps->max_priority; i >=  0; i-- ){
		if( !list_empty (&ps -> lists[i]) ){
			el = list_front (&ps -> lists[i]);
			th = list_entry (el, struct thread, elem);
			break;
		}
	}
	
	return th;
}

/* Checks if scheduler contains the thread */
bool 
ps_contains (struct priority_scheduler * ps, struct thread * th){
	if( th->pss == ps ) 
		return true;
	else 
		return false;
}

/* Updates scheduler */
void 
ps_update_auto ( struct thread * th){
	struct priority_scheduler * ps = th->pss;

	if( ps != NULL ){
		list_remove (&th->elem);
		ps->size--;
		ps_push (ps, th);
	}
}

/* Updates scheduler */
void 
ps_update (struct priority_scheduler * ps, struct thread * th){
	ASSERT( th->status == THREAD_READY );
		
	list_remove (&th->elem);
	ps->size--;
	ps_push (ps, th);
}
