#include "vm/swap.h"
#include "devices/block.h"
#include "lib/kernel/bitmap.h"
#include "lib/kernel/hash.h"

/* Swap partition */
static struct block *swap;

/* Bitmap of free swap slots. */
static struct bitmap free_swap_bitmap;

struct swap_slt{

};

struct swap_slt * swap_slt(){
	block_sector_t swap_addr; //address on the swap
	uint8_t *upage; //page we dumped here
    struct thread * thread; //thread the page belongs to

	struct hash_elem hash_elem;
	struct list_elem list_elem;
}

block_sector_t swap_find_free(){
	
}

bool swap_store(struct swap_slt * swap_slt){
	block_sector_t swap_addr = swap_find_free();
	swap -> write( swap_addr, swap_slt -> addr);
	swap_slt -> swap_addr = swap_addr;
}

struct swap_slt * swap_load(){

}

void swap_init(){
	swap = block_get_role (BLOCK_SWAP);
	//bitmap_init(&free_swap_bitmap);
}