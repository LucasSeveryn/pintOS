#include "threads/priority_scheduler.h"
#include <stdio.h>

#define PS_DEBUG false

void ps_init( struct priority_scheduler * ps ){
	ps -> max_priority = PRI_MIN;
	ps -> size = 0;

	int i;
	for( i = PRI_MAX; i >=  0; i-- ){
		list_init( &ps -> lists[i] );
		if(PS_DEBUG) printf("List for priority %d, initialized.\n", i);
	}
	if(PS_DEBUG) printf("ps_init finished");
}

bool ps_empty( struct priority_scheduler * ps ){
	return ps -> size == 0;
}

/* Priority Queue insertion */
void ps_push( struct priority_scheduler * ps, struct thread * th ){
	ps -> size++;
	list_push_back( &ps -> lists[th -> priority], &th -> elem );
	if(PS_DEBUG)printf("Inserting thread with priority %d; %s.\n", th -> priority, th -> name);

	if (th->priority > ps->max_priority) {
		ps->max_priority = th->priority;
	}

	// we cannot use this since in next_thread_to_run there's no runnig_thread
	/*if( th -> priority > thread_get_priority() ){
		if(PS_DEBUG)printf("Thread's priority %d greater then current running priority %d; %s. Yielding. \n", th -> priority, ps -> max_priority, th -> name );

			ps -> max_priority = th -> priority;
			if( thread_current() != th && ps -> size > 1 ) thread_yield();
	}*/
	if(PS_DEBUG) printf("Ps size: %d\n", ps -> size );
	if(PS_DEBUG) printf("ps_push finished");
}

/* Highest priority */
struct thread * ps_pop( struct priority_scheduler * ps ){
	ps -> size--;
	//if(PS_DEBUG) printf("ps_pop started");
	struct list_elem * el;
	struct thread * th;
	int i;
	for( i = ps -> max_priority; i >=  0; i-- ){
		if( !list_empty( &ps -> lists[i] ) ){
			el = list_pop_front( &ps -> lists[i] );
			th = list_entry( el, struct thread, elem );
			//if(PS_DEBUG) printf("Popping tread with highest priority (%d); %s. \n", th -> priority, th -> name );
			int j;
			for( j = i; j >=0 ; j-- ){
				if( !list_empty( &ps -> lists[j] ) ){
					ps -> max_priority = j;
					break;
				}
			}
			break;
		}
	}
	//f(PS_DEBUG) printf("Ps size: %d\n", ps -> size );
	//if(PS_DEBUG) printf("ps_pop finished");
	return th;
}

struct thread * ps_pull( struct priority_scheduler * ps ){
	struct list_elem * el;
	struct thread * th;
	int i;
	for( i = ps -> max_priority; i >=  0; i-- ){
		if( !list_empty( &ps -> lists[i] ) ){
			el = list_front( &ps -> lists[i] );
		}
	}
	th = list_entry( el, struct thread, elem );

	if(PS_DEBUG) printf("Pulling tread with highest priority (%d); %s. \n", th -> priority, th -> name );
	
	return th;
}

void ps_update( struct priority_scheduler * ps, struct thread * th){
	if(PS_DEBUG)printf("Updating tread's position in the ps; priority (%d); %s. \n", th -> priority, th -> name );
	list_remove( &th -> elem );
	ps_push( ps, th );
}
