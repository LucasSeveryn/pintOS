#ifndef __VM_FRAME_H
#define __VM_FRAME_H

#include <hash.h>
#include "vm/page.h"

struct frame{
	void *addr;
	void *upage;
	struct thread *thread;
	struct origin_info *origin;

	struct hash_elem hash_elem;
};

void frame_init(void);
void *evict(void *, struct thread *);
void *frame_get(void *, bool);
bool frame_free(void *);
struct frame *frame_find(void *);


#endif /* vm/frame.h */
