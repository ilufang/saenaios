/**
 * @file kmalloc.h
 *
 * kmalloc implementation and memory management
 */

#ifndef KMALLOC_H_
#define KMALLOC_H_

#include "page_table.h"

#define KMALLOC_MAX 4194304

typedef struct page {
	char page_size;	///< 0 for not enabled; 1 for 4KB page; 2 for 4MB page
	int start_addr;	///< starting address of the page
	int curr_addr;	///< starting address of not used memory
} page_t;

typedef struct double_list {	///< doubly linked list
	struct page* ptr;			///< pointer to the page struct
	int free_size;	///< size of usable space
	struct double_list* prev;	///< pointer to the previous list item; NULL for first item
	struct double_list* next;	///< pointer to the next list item; NULL for last item
} double_list_t;

typedef struct slab {
	int page_count;		///< number of pages in slab
	struct double_list *free_slab; 		///< pointer to free_slab list
	struct double_list *partial_slab; 	///< pointer to partially used slab
	struct double_list *full_slab;		///< pointer to 
} slab_t;

void kmalloc_init();

int kmalloc(size_t size);

int get_free_page(slab_t slab, int addr, size_t size);

#endif
