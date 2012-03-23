#ifndef __VM_FRAME_H
#define __VM_FRAME_H

#include <hash.h>
#include "vm/page.h"
#include "vm/swap.h"

struct frame {
	void *addr;
	void *upage;
	struct thread *thread;
	struct origin_info *origin;
	bool pinned;

	struct hash_elem hash_elem;
};

void frame_init(void);
void evict(void);
void *frame_get(void *, bool, struct origin_info * );
bool frame_free(void *);
void frame_pin(void *, int);
void frame_unpin_kernel(void *, int);
void frame_pin_kernel(void *, int);
void frame_unpin(void *, int);
void lock_frames (void);
void unlock_frames (void);

struct frame *frame_find(void *);


#endif /* vm/frame.h */
