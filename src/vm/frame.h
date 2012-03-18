#ifndef __VM_FRAME_H
#define __VM_FRAME_H

#include "lib/kernel/hash.h"

struct frame{
	void * addr;
	void * upage;
	struct thread * thread;
	
	struct hash_elem hash_elem;
};

void frame_init(void);
void * evict(void *, struct thread *);
void * frame_get(void *, bool);
bool frame_free(void *);
struct frame *  frame_find(void *);


#endif /* vm/frame.h */
