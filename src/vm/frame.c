#include "vm/frame.h"
#include <stdio.h>
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"

static struct hash frames;
static struct lock frames_lock;

static bool DEBUG = false;

void lock_frames (void);
void unlock_frames (void);
bool frame_less (const struct hash_elem *, const struct hash_elem *, void *);
unsigned frame_hash (const struct hash_elem *, void *);
void page_dump (struct frame *);
void frame_set_pin (void *, bool);
int get_class (uint32_t * , const void *);

void
lock_frames (){
	lock_acquire (&frames_lock);
}

void unlock_frames (){
	lock_release (&frames_lock);
}

void
frame_init (){
	hash_init (&frames, frame_hash, frame_less, NULL);
	lock_init (&frames_lock);
}

/*
Allocates a new page, and adds it to the frame table
*/
void *
frame_get (void * upage, bool zero, struct origin_info *origin){
	void * kpage = palloc_get_page ( PAL_USER | (zero ? PAL_ZERO : 0) );
	struct thread * t = thread_current ();

	/* There is no more free memory, we need to free some */
	if(kpage == NULL) {
		lock_frames ();
		evict ();
		kpage = palloc_get_page ( PAL_USER | (zero ? PAL_ZERO : 0) );
		unlock_frames ();
	}

	/* We succesfully allocated space for the page */
	if(kpage != NULL){
		struct frame * frame = (struct frame*) malloc (sizeof (struct frame));
		frame -> addr = kpage;
		frame -> upage = upage;
		frame -> origin = origin;
		frame -> thread = t;
		frame -> pinned = false;

		lock_frames ();
		hash_insert (&frames, &frame -> hash_elem);
		unlock_frames ();
	}

	return kpage;
}

/*
Remove a frame, and clean up after it.
*/
bool
frame_free (void * addr){
	struct frame * frame;
	struct hash_elem * found_frame;
	struct frame frame_elem;
	frame_elem.addr = addr;

	found_frame = hash_find (&frames, &frame_elem.hash_elem);
	if(found_frame != NULL){
		frame = hash_entry (found_frame, struct frame, hash_elem);

		palloc_free_page (frame->addr); //Free physical memory
		hash_delete (&frames, &frame->hash_elem); //Free entry in the frame table
		free (frame); //Delete the structure

		return true;
	} else {
		return false;
		//Wellll... Nothing to do here :)
	}
}

/*
Sets frame's pin to the given value
*/
void
frame_set_pin (void * kpage, bool pinval){
    struct frame * frame = frame_find (kpage);
    if(frame == NULL)  {
    	return;
    }
    frame->pinned = pinval;
}

/*
Pins a frame using user virtual memory address
*/
void
frame_pin (void * vaddr, int l){
	struct thread * t = thread_current ();
	int i;
	int it = l / PGSIZE;
	if(l % PGSIZE) it++;
	for(i = 0; i < it; i++){
		sema_down (&t->pagedir_mod);
		void * kpage = pagedir_get_page (t->pagedir, pg_round_down(vaddr) + i * PGSIZE);
		sema_up (&t->pagedir_mod);
		if(kpage == 0 || pg_ofs (kpage) != 0) return;
		frame_set_pin (kpage, true);
	}
}

/*
Unpins a frame using user virtual memory address
*/
void 
frame_unpin (void * vaddr, int l){
	struct thread * t = thread_current ();
	int i;
	int it = l / PGSIZE;
	if(l % PGSIZE) it++;
	for(i = 0; i < it; i++){
		sema_down (&t->pagedir_mod);
		void * kpage = pagedir_get_page (t->pagedir, pg_round_down (vaddr) + i * PGSIZE);
		sema_up (&t->pagedir_mod);
		if(kpage == 0 || pg_ofs (kpage) != 0)  return;
	    frame_set_pin (kpage, false);
	}
}

/*
Pins a frame using physical memory address
*/
void
frame_pin_kernel (void * kpage, int l){
	int i;
	int it = l / PGSIZE;
	if(l % PGSIZE) it++;
	for(i = 0; i < it; i++){
		if( kpage == 0 || pg_ofs (kpage) != 0 ) return;
	    frame_set_pin (kpage, true);
	}
}

/*
Unpins a frame using physical memory address
*/
void 
frame_unpin_kernel (void * kpage, int l){
	int i;
	int it = l / PGSIZE;
	if(l % PGSIZE) it++;
	for(i = 0; i < it; i++){
	    if( kpage == 0 || pg_ofs (kpage) != 0 ) return;
	    frame_set_pin (kpage, false);
	}
}

/*
Looks for a frame with the given physical address, if no such frame exists return NULL
*/
struct frame *
frame_find (void * addr){
	struct frame * frame;
	struct hash_elem * found_frame;
	struct frame frame_elem;
	frame_elem.addr = addr;

	found_frame = hash_find (&frames, &frame_elem.hash_elem);
	if(found_frame != NULL){
		frame = hash_entry (found_frame, struct frame, hash_elem);
		return frame;
	} else {
		return NULL;
	}
}

/*
Comparision function
*/
bool
frame_less (const struct hash_elem *a_, const struct hash_elem *b_	, void *aux UNUSED){
	const struct frame * a = hash_entry (a_, struct frame, hash_elem);
	const struct frame * b = hash_entry (b_, struct frame, hash_elem);
	return a->addr < b->addr;
}

/*
Hash function
*/
unsigned
frame_hash(const struct hash_elem *fe, void *aux UNUSED){
	const struct frame * frame = hash_entry (fe, struct frame, hash_elem);
	return hash_int ((unsigned)frame->addr);
}


/*
Determines a class the page belongs to
*/
int
get_class (uint32_t * pd, const void * page) {
	void * kpage = pagedir_get_page (pd, page);
	if(kpage == NULL) return -1;

	bool dirty = pagedir_is_dirty (pd, page);
	bool accessed = pagedir_is_accessed (pd, page);

	return (accessed) ? (( dirty ) ? 4 : 2) : (( dirty ) ? 3 : 1);
}


/*
Performs actual eviction of a page.
*/
void
page_dump( struct frame * frame ){
	bool dirty = pagedir_is_dirty (frame->thread->pagedir, frame->upage);
	struct suppl_page * suppl_page = NULL;

	if (dirty)
	{
		if (frame->origin != NULL && frame->origin->location == FILE)
		{
			filesys_lock_acquire ();
			frame_pin (frame->upage, PGSIZE);
			file_write_at (frame->origin->source_file, frame->addr, frame->origin->zero_after, frame->origin->offset);
			frame_unpin (frame->upage, PGSIZE);
			filesys_lock_release ();

			suppl_page = new_file_page (frame->origin->source_file, frame->origin->offset, frame->origin->zero_after, frame->origin->writable, FILE);
		} else {
			struct swap_slt * swap_el = swap_slot(frame);

			frame_pin (frame->upage, PGSIZE);
			swap_store (swap_el);
			frame_unpin (frame->upage, PGSIZE);

			suppl_page = new_swap_page (swap_el);
		}
	}
	else
	{
		if (frame->origin != NULL)
		{
			suppl_page = new_file_page (frame->origin->source_file, frame->origin->offset, frame->origin->zero_after, frame->origin->writable, frame->origin->location);
		} else {
			suppl_page = new_zero_page ();
		}
	}

	sema_down (&frame->thread->pagedir_mod);
	pagedir_clear_page (frame->thread->pagedir, frame->upage);
	pagedir_set_page_suppl (frame->thread->pagedir, frame->upage, suppl_page);
	sema_up (&frame->thread->pagedir_mod);

}

/*
Selects a frame to evict
*/
void
evict(){
	struct hash_iterator it;
	void * kpage = NULL;
	struct frame *f = NULL;
	int i;
	
	/* Second chance page replacement */
	for(i = 0; i < 2 && kpage == NULL; i++){
		hash_first (&it, &frames);

		/* Look for an element in the lowest class */
		while(kpage == NULL && hash_next (&it)){
			f = hash_entry (hash_cur (&it), struct frame, hash_elem);
			if(f->pinned) continue;
			sema_down (&f->thread->pagedir_mod);
			int class = get_class (f->thread->pagedir, f->upage);
			sema_up (&f->thread->pagedir_mod);
			if(class == 1){
				page_dump (f);
				kpage = f->addr;
			}
		}

		hash_first (&it, &frames);

		/* Look for an element in the higher class, at the same time lowering classes of passed elements */
		while(kpage == NULL && hash_next (&it)){
			f = hash_entry (hash_cur (&it), struct frame, hash_elem);
			if(f->pinned) continue;
			sema_down (&f->thread->pagedir_mod);
			int class = get_class (f->thread->pagedir, f->upage);
			sema_up (&f->thread->pagedir_mod);
			if(class == 3){
				page_dump (f);
				kpage = f->addr;
			} else if(class > 0){
				pagedir_set_accessed (f->thread->pagedir, f->upage, false);
			}
		}
	}

	palloc_free_page (f->addr); /* Free physical memory */
	hash_delete (&frames, &f->hash_elem); /* Free entry in the frame table */
	free (f); /* Delete the structure */
}
