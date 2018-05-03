#include "kmalloc.h"

static struct memory_list mem_info[SLAB_NUM];
static struct memory_list *free_list;
static struct memory_list *alloc_list;

static char status = 0;								// indicate if memory pool is initialized
static int 	start_addr;
static int 	cur_addr = 0;							// indicate virtual addr for next allocated starting page addr

void kmalloc_init() {

	int i; 			// iterator
	int addr = 0;	// request addr from page;

	for (i = 1; i < SLAB_NUM; i++) {
		mem_info[i].status = 0;
		mem_info[i].size = 0;
		mem_info[i].prev = NULL;
		mem_info[i].info = NULL;
		mem_info[i].next = NULL;
	}

	// REQUESTE 3 4MB PAGES
	page_alloc_4MB(&addr);
	start_addr = addr;
	cur_addr = addr + (4 * MEGA_BYTE);
	page_dir_add_4MB_entry(addr, addr, PAGE_DIR_ENT_PRESENT | PAGE_DIR_ENT_RDWR |
						PAGE_DIR_ENT_SUPERVISOR | PAGE_DIR_ENT_4MB |
						PAGE_DIR_ENT_GLOBAL);

	status = 1;
	get_free_page();
	get_free_page();
/*
	memory_pool[0].status = 0;
	memory_pool[0].start_addr = start_addr;
	memory_pool[0].size = SLAB_NUM;
*/
	((malloc_info_t*)start_addr)->start_addr = start_addr;
	((malloc_info_t*)start_addr)->size = SLAB_NUM;
	((malloc_info_t*)start_addr)->status = 0;

	free_list = &(mem_info[0]);
	free_list->status = 1;
	free_list->size = SLAB_NUM;
	free_list->prev = NULL;
	free_list->info = ((malloc_info_t*)start_addr);
	((malloc_info_t*)start_addr)->link = free_list;
	free_list->next = NULL;

	alloc_list = NULL;
	// alloc_list->status = 1;
	// alloc_list->size = 0;
	// alloc_list->prev = NULL;
	// alloc_list->info = NULL;
	// alloc_list->next = NULL;

	status = 1;
}

int get_free_page() {

	if (status == 0) {
		return -ENOBUFS;
	}

	if (cur_addr - start_addr >= (12 * MEGA_BYTE)) {
		return -ENOBUFS;
	}

	int addr = 0;
	page_alloc_4MB(&addr);
	page_dir_add_4MB_entry(cur_addr, addr, PAGE_DIR_ENT_PRESENT | PAGE_DIR_ENT_RDWR |
					PAGE_DIR_ENT_SUPERVISOR | PAGE_DIR_ENT_4MB |
					PAGE_DIR_ENT_GLOBAL);
	cur_addr += (4 * MEGA_BYTE);

	return 0;
}

int ceiling_division(int x, int y) {
	if ((x % y) != 0)
		return (x/y) + 1;
	else
		return x/y;
}
void* kmalloc(size_t size) {

	if (status == 0) {
		kmalloc_init();
	}

	int num_slab;
	int addr;
	int i;

	memory_list_t *temp, *temp2;
	temp = free_list;

	num_slab = ceiling_division((size + INFO_SIZE),SLAB_SIZE);

	while(temp != NULL) {
		if (temp->size >= num_slab)
			break;
		temp = temp->next;
	}

	if (temp->size - num_slab == 0) {

		addr = temp->info->start_addr;

		if (temp->prev != NULL)
			temp->prev->next = temp->next;
		if (temp->next != NULL)
			temp->next->prev = temp->prev;

		temp2 = alloc_list;

		while (temp2->next != NULL) {
			temp2 = temp2->next;
		}

		temp2->next = temp;
		temp->prev = temp2;
		temp->next = NULL;
		temp->info->status = 1;

		if (temp == free_list) {
			free_list = NULL;
		}

	} else {
		addr = temp->info->start_addr;
		// temp for allocated memory, find new place for free slabs
		malloc_info_t *new_info;
		new_info = (malloc_info_t*)(addr + (num_slab*SLAB_SIZE));
		for (i = 1; i < SLAB_NUM; i++) {
			if (mem_info[i].status == 0) {
				temp2 = &(mem_info[i]);
				temp2->status = 1;
				goto buf_found;
			}
		}
		// Out of memory
		errno = ENOMEM;
		return NULL;
	buf_found:
		temp2->size = temp->size - num_slab;
		temp2->prev = temp->prev;
		temp2->info = new_info;
		temp2->info->start_addr = temp->info->start_addr + num_slab*SLAB_SIZE;
		temp2->info->size = temp2->size;
		temp2->info->status = 1;
		temp2->info->link = temp2;
		temp2->next = temp->next;

		if (temp->prev != NULL)
			temp->prev->next = temp2;
		if (temp->next != NULL)
			temp->next->prev = temp2;

		temp->size = num_slab;
		temp->info->start_addr = addr;
		temp->info->size = num_slab;
		temp->info->status = 1;

		if (temp == free_list) {
			free_list = temp2;
		}

		temp2 = alloc_list;
		if (temp2 != NULL) {
			while (temp2->next != NULL) {
				temp2 = temp2->next;
			}

			temp2->next = temp;
			temp->prev = temp2;
			temp->next = NULL;
		} else {
			alloc_list = temp;
			temp->prev = NULL;
			temp->next = NULL;
		}
	}

	return (void*)(addr+INFO_SIZE);
}

int kfree(void* ptr) {

	if (((malloc_info_t*)(ptr - INFO_SIZE))->status != 1)
		return -EINVAL;

	malloc_info_t *info = (malloc_info_t*)(ptr - INFO_SIZE);
	int offset = info->size * SLAB_SIZE;
	memory_list_t *link = info->link;
	memory_list_t *temp = free_list;
	if (temp != NULL) {
		while(temp->next != NULL) {
			if (temp->info->start_addr + offset == info->start_addr)
				break;
			temp = temp->next;
		}

		if (temp->next != NULL) {
			//TODO
			temp->size += link->size;
			temp->info->size = temp->size;
			memset((char *) info->start_addr,0, info->size*SLAB_SIZE);

		} else if (temp->info->start_addr + offset == info->start_addr){
			info->status = 0;
			temp->next = link;
			link->prev = temp;
			link->next = NULL;
		} else {
			temp->size += link->size;
			temp->info->size = temp->size;
			memset((char *) info->start_addr,0, info->size*SLAB_SIZE);
		}
	} else {
		info->status = 0;
		free_list = link;
		link->prev = NULL;
		link->next = NULL;
	}

	return 0;
}

void* malloc(size_t size){
	return kmalloc(size);
}

void* calloc(size_t count, size_t size){
	void* temp = kmalloc(count*size);
	if (temp){
		memset(temp, 0, count*size);
	}
	return temp;
}

void free(void* ptr){
	free(ptr);
}
