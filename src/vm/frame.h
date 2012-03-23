#ifndef __VM_FRAME_H
#define __VM_FRAME_H

#include <hash.h>
#include "vm/page.h"
#include "vm/swap.h"

struct frame {
	void *addr;					/* Physical address of the page */
	void *upage;				/* User virtual address of the page*/
	struct thread *thread;		/* Thread the page belongs to*/
	struct origin_info *origin; /* Source of origin*/
	bool pinned;				/* Pin - makes the page not evictable */

	struct hash_elem hash_elem;
};

void frame_init (void);
void evict (void);
struct frame *frame_find (void *);
void* frame_get (void *, bool, struct origin_info *);
bool frame_free (void *);

/* Frames security functions */
void frame_pin (void *, int);
void frame_unpin (void *, int);
void frame_unpin_kernel (void *, int);
void frame_pin_kernel (void *, int);
void lock_frames (void);
void unlock_frames (void);



#endif /* vm/frame.h */
