#include "vm/swap.h"
#include "lib/kernel/bitmap.h"

/* List of free swap slots. */
static struct list free_swap_list;

/* Swap partition */
static struct disk *swap_parition;

/* Bitmap of free swap slots. */
static struct bitmap free_swap_bitmap;

void swap_init(){
	//bitmap_init(&free_swap_bitmap);
}