#include "vm/swap.h"
#include "lib/kernel/bitmap.h"

/* List of free swap slots. */
static struct list free_swap_list;

/* Bitmap of free swap slots. */
static struct bitmap free_swap_bitmap;

void swap_init(){
	
}