#include "vm/page.h"
#include "threads/malloc.h"

struct suppl_page * new_zero_page() {
	struct suppl_page *new_page = (struct suppl_page *) malloc (sizeof (struct suppl_page));
	new_page->location = ZERO;
	new_page->origin = NULL;
	new_page->swap_elem = NULL;
	return new_page;
}

struct suppl_page * new_file_page(struct file * source, off_t offset, size_t zero_after, bool writable) {
	struct suppl_page *new_page = (struct suppl_page *) malloc (sizeof (struct suppl_page));
	new_page->location = FILE;

	struct origin_info *origin = (struct origin_info *) malloc (sizeof (struct origin_info));
	origin->source_file = source;
	origin->offset = offset;
	origin->zero_after = zero_after;
	origin->writable = writable;

	new_page->origin = origin;
	new_page->swap_elem = NULL;
	return new_page;
}

struct suppl_page * new_swap_page(struct swap_block *swap_location) {
	struct suppl_page *new_page = (struct suppl_page *) malloc (sizeof (struct suppl_page));
	new_page->location = SWAP;
	new_page->origin = NULL;
	new_page->swap_elem = swap_location;
	return new_page;
}