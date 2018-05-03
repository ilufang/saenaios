/**
 *	@file kmalloc.h
 *
 *	kmalloc implementation and memory management
 */

#ifndef KMALLOC_H_
#define KMALLOC_H_

#include "../boot/page_table.h"

#define MEGA_BYTE 	0x00100000
#define SLAB_SIZE 	0x00000200 				///< SLAB_SIZE = 512 bytes
#define KMEM_POOL 	0x00C00000				///< memory pool size = 12MB
#define SLAB_NUM	(KMEM_POOL/SLAB_SIZE)	///< total number of slabs in memory pool = 8192
#define INFO_SIZE 	sizeof(malloc_info_t)

typedef struct malloc_info{
	int start_addr;	///< the starting address of space
	int size;		///< number of slabs;
	char status; 	///< 0 for free, 1 for allocated
	struct memory_list *link;
} malloc_info_t;

typedef struct memory_list {
	char status;	///< 0 for not used;
	int size;
	struct memory_list *prev;		///< addr to the previous linked memory
	struct malloc_info *info;		///< malloc info
	struct memory_list *next;		///< pointer to the next item, NULL for last item
} memory_list_t;

void kmalloc_init();

int get_free_page();

int ceiling_division(int x, int y);

void* kmalloc(size_t size);

int kfree(void* ptr);

void* malloc(size_t size);

void* calloc(size_t count, size_t size);

void free(void* ptr);

#endif
