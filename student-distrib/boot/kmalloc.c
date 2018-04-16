#include "kmalloc.h"

static struct page page_4KB_array[3072]; // Currently hold 12MB page info
static struct page page_4MB_array[3];	// Currently hold 12MB page info
static struct double_list page_list[3075]; // Hold all page_list
static struct slab slab_array[3]; 		// each slab holds max 1024 pages;
static char status = 0;			// indicate if mem space for kmalloc is initialized, -1 for needs to be clear

void kmalloc_init() {
	int i; // iterator

	for (i = 0; i < 3072; i++) {
		page_4KB_array[i].page_size = 0;
		page_4KB_array[i].start_addr = 0;
		page_4KB_array[i].curr_addr = 0;
		page_list[i].prev = 0;
		page_list[i].ptr = 0;
		page_list[i].free_size = 4096;
		page_list[i].next = 0;
	}

	for (i = 0; i < 3; i++) {
		page_4MB_array[i].page_size = 0;
		page_4MB_array[i].start_addr = 0;
		page_4MB_array[i].curr_addr = 0;
		page_list[i+3072].prev = 0;
		page_list[i+3072].ptr = 0;
		page_list[i+3072].free_size = 4096;
		page_list[i+3072].next = 0;
		slab_array[i].page_count = 0;
		slab_array[i].free_slab = 0;
		slab_array[i].partial_slab = 0;
		slab_array[i].full_slab = 0;
	}

	status = 1;
}

int kmalloc(size_t size) {
	
	int i;			//iterator
	int addr = 0; 	//addr for return value
	// test if argument is right
	if (size < 1 || size > KMALLOC_MAX) {
		return -EINVAL;
	}
	// initialize the memory space if it is not initialized
	if (status == 0 || status == -1) {
		kmalloc_init();
	}

	i = 0;
	while(i < 3) {
// if page in slab == 0
		if (slab_array[i].page_count == 0) {
			get_free_page(slab_array[i], addr, size);
			slab_array[i].free_slab->free_size -= size;
			slab_array[i].free_slab->ptr->curr_addr += size;
			slab_array[i].partial_slab = slab_array[i].free_slab;
			slab_array[i].free_slab = 0;
			return addr;
		}

// if page_count > 0 and page_count <= 1024
		if (slab_array[i].page_count > 0 && slab_array[i].page_count < 1024) {
			if (slab_array[i].partial_slab == 0) {
				// if there's no partial slab
				if (slab_array[i].free_slab == 0) {
					// if there's no free slab
					get_free_page(slab_array[i], addr, size);
					slab_array[i].free_slab->free_size -= size;
					slab_array[i].free_slab->ptr->curr_addr += size;
					slab_array[i].partial_slab = slab_array[i].free_slab;
					slab_array[i].free_slab = 0;
					return addr;
				} else {
					// if there's free slab
				}
			} else {
				// if there's partial slab
			}
		}
// if page_count >= 1024
		if (slab_array[i].page_count >= 1024) {
			i++;
		}
// no buffer allocated
		if (i == 2) {
			return -ENOBUFS;
		}
	}

	return 0;
}

int get_free_page(slab_t slab, int addr, size_t size) {

	int i, j;
	if (size > 4096) {
		page_alloc_4MB(&addr);
		for (i = 0; i < 3; i++) {
			if (page_4MB_array[i].page_size == 0) {
				page_4MB_array[i].page_size = 2;
				page_4MB_array[i].start_addr = addr;
				page_4MB_array[i].curr_addr = addr;
				break;
			}
			if (i == 2) {
				return -ENOBUFS;
			}
		}
		for (j = 0; j < 3075; j++) {
			if (page_list[j].ptr == 0) {
				page_list[j].ptr = &(page_4MB_array[i]);
				page_list[j].free_size = KMALLOC_MAX;
				slab.free_slab = &(page_list[j]);
				slab.page_count++;
				break;
			}
			if (j == 2) {
				return -ENOBUFS;
			}
		}
	} else{
		page_alloc_4KB(&addr);
		for (i = 0; i < 3072; i++) {
			if (page_4KB_array[i].page_size == 0) {
				page_4KB_array[i].page_size = 1;
				page_4KB_array[i].start_addr = addr;
				page_4KB_array[i].curr_addr = addr;
				break;
			}
			if (i == 3071) {
				return -ENOBUFS;
			}
		}
		for (j = 0; j < 3075; j++) {
			if (page_list[j].ptr == 0) {
				page_list[j].ptr = &(page_4KB_array[i]);
				page_list[j].free_size = 4096;
				slab.free_slab = &(page_list[j]);
				slab.page_count++;
				break;
			}
			if (j == 3071) {
				return -ENOBUFS;
			}
		}
	}
	return 0;
}
