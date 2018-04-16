#include "kmalloc.h"

static struct page page_4KB_array[3072]; // Currently hold 12MB page info
static struct page page_4MB_array[3];	// Currently hold 12MB page info
static struct double_list page_list[3075]; // Hold all page_list
static struct slab slab_array[3]; 		// each slab holds max 1024 pages;
static char status = 0;			// indicate if mem space for kmalloc is initialized

void kmalloc_init() {
	int i; // iterator

	for (i = 0; i < 3072; i++) {
		page_4KB_array[i].page_size = 0;
		page_4KB_array[i].free_size = 4096;
		page_4KB_array[i].start_addr = 0;
		page_4KB_array[i].curr_addr = 0;
		page_list[i].prev = 0;
		page_list[i].ptr = 0;
		page_list[i].next = 0;
	}

	for (i = 0; i < 3; i++) {
		page_4MB_array[i].page_size = 0;
		page_4MB_array[i].free_size = (4096*1024);
		page_4MB_array[i].start_addr = 0;
		page_4MB_array[i].curr_addr = 0;
		page_list[i+3072].prev = 0;
		page_list[i+3072].ptr = 0;
		page_list[i+3072].next = 0;
		slab_array[i].page_count = 0;
	}

	status = 1;
}