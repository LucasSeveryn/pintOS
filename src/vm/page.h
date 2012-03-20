#ifndef __VM_PAGE_H
#define __VM_PAGE_H

#include "vm/swap.h"
#include "filesys/file.h"
#include "threads/vaddr.h"
#include <stdbool.h>
#include <stdlib.h>

#define STACK_BOTTOM ((void *) 0xbf800000)

enum page_type
	{
		FILE,
		EXEC,
		SWAP,
		ZERO,
		MMAP
	};

struct suppl_page
	{
		// Where the required data is located
		enum page_type location;
		// Where on the filesystem the data originally resides
		struct origin_info *origin;
		// If we are located in the swap this elements points to swap
		struct swap_slt *swap_elem;
	};

struct origin_info
	{
		struct file *source_file;
		off_t offset;
		size_t zero_after;
		bool writable;
		enum page_type location;
	};

struct suppl_page * new_file_page (struct file *, off_t, size_t, bool, enum page_type);
struct suppl_page * new_swap_page (struct swap_slt *);
struct suppl_page * new_zero_page (void);
inline bool is_stack_access(void *esp, void *address);

#endif /* vm/page.h */