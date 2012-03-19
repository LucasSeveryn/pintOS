#ifndef __VM_PAGE_H
#define __VM_PAGE_H

#include "vm/swap.h"
#include "filesys/file.h"
#include <stdbool.h>
#include <stdlib.h>

enum page_type
	{
		FILE,
		SWAP,
		ZERO
	};

struct suppl_page
	{
		// Where the required data is located
		enum page_type location;
		// Where on the filesystem the data originally resides
		struct origin_info *origin;
		// If we are located in the swap this elements points to swap
		struct swap_block *swap_elem;
	};

struct origin_info
	{
		struct file *source_file;
		off_t offset;
		size_t zero_after;
		bool writable;
	};

struct suppl_page * new_file_page (struct file *, off_t, size_t, bool);
struct suppl_page * new_swap_page (struct swap_block *);
struct suppl_page * new_zero_page (void);

#endif /* vm/page.h */