#include "page_table.h"
#include "../proc/signal.h"

#define ALLOCATABLE_4MB_START_INDEX	0x4	///< new pages will allocate from here

#define PAGE_4MB 	0x400000

#define PAGE_4KB 	0x1000

#define MANYOUSHU_PAGE_START_ADDR	0xC00000

#define GET_DIR_INDEX(x) (x / PAGE_4MB)

#define GET_TAB_INDEX(x) ((x / PAGE_4KB) & (0x3FF))

#define GET_MEM_MAP_INDEX(x) (x / PAGE_4MB)

#define GET_MANYOU_INDEX(x) ((x / PAGE_4KB) & (0x3FF))

//128MB
#define USER_PAGE_TABLE_VIR_ADDR	0x8000000

static page_4MB_descriptor_t page_phys_mem_map[MAX_DYNAMIC_4MB_PAGE];

static page_4KB_descriptor_t manyoushu_mem_table[1024];

static int page_4KB_allocation_index = 0;

static int page_4MB_allocation_index = ALLOCATABLE_4MB_START_INDEX;

static page_directory_t page_directory;

static page_table_t ece391_init_page_table;

static page_table_t manyoushu_page_table;

int get_phys_mem_reference_count(int physical_addr){
	// align to 4MB
	int mem_index = GET_MEM_MAP_INDEX(physical_addr);

	if ((page_dir_index == 0) || (page_dir_index >= MAX_DYNAMIC_4MB_PAGE))
		return -EINVAL;

	if (page_dir_index == 3){
		// manyoushu page
		int mem_table_index = GET_MANYOU_INDEX(physical_addr);
		return manyoushu_mem_table[mem_table_index].count;
	}else{
		return page_phys_mem_map.count;
	}
}

int _page_alloc_get_4MB(){
	int sanity_count = 0;
	// find next free 4MB page
	while (page_phys_mem_map[page_4MB_allocation_index].count > 0){
		// increment index
		++page_4MB_allocation_index;
		// wrap around
		if (page_4MB_allocation_index > MAX_DYNAMIC_4MB_PAGE){
			page_4MB_allocation_index = ALLOCATABLE_4MB_START_INDEX;
		}
		// don't go to a dead loop
		++sanity_count;
		if (sanity_count > (MAX_DYNAMIC_4MB_PAGE - ALLOCATABLE_4MB_START_INDEX)){
			// nah, the physical memory is full
			return -ENOMEM;
		}
	}
	// found a good one, mark as used
	page_phys_mem_map[page_4MB_allocation_index].count++;
	return page_4MB_allocation_index * PAGE_4MB;
}

// should be only used for C00000 - 1000000 memory space
int _page_alloc_get_4KB(){
	int sanity_count = 0;
	while (manyoushu_mem_table[page_4KB_allocation_index].count>0){
		// increment index
		++page_4KB_allocation_index;
		// wrap around
		if (page_4KB_allocation_index > 1024){
			page_4KB_allocation_index = 0;
		}
		// don't go to a dead loop
		++sanity_count;
		if (sanity_count > 1024){
			// nah, the 4KB pages are all out
			return -ENOMEM;
		}
	}
	// found a good one, mark as used
	manyoushu_mem_table[page_4KB_allocation_index].count ++;

	return MANYOUSHU_PAGE_START_ADDR + page_4KB_allocation_index * PAGE_4KB;
}

int _page_alloc_add_refer_4MB(int addr){
	// check if the physical address is really allocated
	if (page_phys_mem_map[GET_DIR_INDEX(addr)].count<=0){
		// not valid
		return -EINVAL;
	}
	// add reference count
	page_phys_mem_map[GET_DIR_INDEX(addr)].count ++;
	return 0;
}

int _page_alloc_add_refer_4KB(int addr){
	// check if the physical address is in the manyoushu 4MB
	if (GET_DIR_INDEX(addr)!=MANYOUSHU_PAGE_START_ADDR){
		return -EINVAL;
	}
	// check if the physical address is really allocated
	if (manyoushu_mem_table[GET_TAB_INDEX(addr)].count<=0){
		return -EINVAL;
	}
	manyoushu_mem_table[GET_TAB_INDEX(addr)].count++;
	return 0;
}

int page_alloc_4MB(int* physical_addr){
	if (!physical_addr){
		return -EINVAL;
	}
	int temp_return;
	if (*physical_addr){
		// means incrementing reference
		return _page_alloc_add_refer_4MB(*physical_addr);
	}else{
		// means mallocing new page
		temp_return = _page_alloc_get_4MB();
		if (temp_return<0){
			return temp_return;
		}else{
			*physical_addr = temp_return;
			return 0;
		}
	}
}

int page_alloc_4KB(int* physical_addr){
	if (!physical_addr){
		return -EINVAL;
	}
	int temp_return;
	if (*physical_addr){
		// means incrementing reference
		return _page_alloc_add_refer_4KB(*physical_addr);
	}else{
		// means mallocing new page
		temp_return = _page_alloc_get_4KB();
		if (temp_return<0){
			return temp_return;
		}else{
			*physical_addr = temp_return;
			return 0;
		}
	}
}

int page_alloc_free_4MB(int physical_addr){
	// never free a kernel page or a manyoushu page
	if ((physical_addr < MANYOUSHU_PAGE_START_ADDR) ||
		(physical_addr > (MANYOUSHU_PAGE_START_ADDR + PAGE_4MB))){
		return -EINVAL;
	}
	// check if the physical page is really in use
	if (page_phys_mem_map[GET_DIR_INDEX(physical_addr)].count<=0){
		return -EINVAL;
	}
	// then decrease the use count
	page_phys_mem_map[GET_DIR_INDEX(physical_addr)].count--;
	return 0;
}

int page_alloc_free_4KB(int physical_addr){
	// the physical address should be in the manyoushu page
	if ((physical_addr > MANYOUSHU_PAGE_START_ADDR)||
		(physical_addr < MANYOUSHU_PAGE_START_ADDR + PAGE_4MB)){
		return -EINVAL;
	}
	// check if the physical 4KB page is really in use
	if (manyoushu_mem_table[GET_TAB_INDEX(physical_addr)].count<=0){
		return -EINVAL;
	}
	// then decrease the use count;
	manyoushu_mem_table[GET_TAB_INDEX(physical_addr)].count --;
	return 0;
}


void page_ece391_init(){
	// clear the page directory
	memset(&page_directory, 0, 4096);

	// clear the page table of 0-4MB
	memset(&ece391_init_page_table, 0, 4096);

	// work around
	page_phys_mem_map_init();

	// create 4KB page for 0-4MB
	page_dir_add_4KB_entry(0x0, (void*)(&ece391_init_page_table), PAGE_DIR_ENT_PRESENT |
							PAGE_DIR_ENT_RDWR | PAGE_DIR_ENT_SUPERVISOR |
							PAGE_DIR_ENT_GLOBAL);
	// create 4MB page for 4-8MB (kernel code)
	page_dir_add_4MB_entry(0x400000, 0x400000, PAGE_DIR_ENT_PRESENT | PAGE_DIR_ENT_RDWR |
							PAGE_DIR_ENT_SUPERVISOR | PAGE_DIR_ENT_4MB |
							PAGE_DIR_ENT_GLOBAL);
	// create 4MB page for 8-12MB (program kernel stacks)
	page_dir_add_4MB_entry(0x800000, 0x800000, PAGE_DIR_ENT_PRESENT | PAGE_DIR_ENT_RDWR |
							PAGE_DIR_ENT_SUPERVISOR | PAGE_DIR_ENT_4MB |
							PAGE_DIR_ENT_GLOBAL);

	// create 4KB page for video memory
	page_tab_add_entry(0xB8000, 0xB8000,PAGE_TAB_ENT_PRESENT | PAGE_TAB_ENT_RDWR |
								PAGE_TAB_ENT_SUPERVISOR | PAGE_TAB_ENT_GLOBAL);

	// create 4KB page table for Manyoushu
	page_dir_add_4KB_entry(USER_PAGE_TABLE_VIR_ADDR,&manyoushu_page_table,
			PAGE_DIR_ENT_PRESENT | PAGE_DIR_ENT_GLOBAL
			| PAGE_DIR_ENT_RDWR | PAGE_DIR_ENT_USER);

	page_turn_on((int)(&page_directory));

	// paging is turned on, but we still have other things to do
	page_kernel_mem_map_init();
	// Initialize global memory blocks
	memset((char *)0x800000, -1, 4<<20);

	signal_init();
}

void page_phys_mem_map_init(){
	int i;
	// initiate physical memory page descriptors
	for (i=0;i<MAX_DYNAMIC_4MB_PAGE;++i){
		page_phys_mem_map[i].flags = 0;
		page_phys_mem_map[i].count = 0;
		page_phys_mem_map[i].pages = NULL;
	}
	// initiate memory map for manyoushu
	for (i=0;i<1024;++i){
		manyoushu_mem_table[i].flags = 0;
		manyoushu_mem_table[i].count = 0;
	}
	page_phys_mem_map[0].count = 1;
	page_phys_mem_map[1].count = 1;
	page_phys_mem_map[2].count = 1;

	manyoushu_mem_table[0xb8].count = 1;
}

void page_kernel_mem_map_init(){
	// first 3 4MB pages are reserved for kernel usage
	page_phys_mem_map[0].flags |= PAGE_DES_KERNEL;
	page_phys_mem_map[1].flags |= PAGE_DES_KERNEL;
	page_phys_mem_map[2].flags |= PAGE_DES_KERNEL;

	// C00000 - 1000000 addresses for 4KB Manyoushu
	page_phys_mem_map[3].flags |= PAGE_DES_RESERVE;
	page_phys_mem_map[3].pages = manyoushu_mem_table;

}

// this function should only be called during initialization
int page_dir_add_4KB_entry(uint32_t virtual_addr, void* new_page_table, int flags){
	// check invalid parameter
	if (!new_page_table){
		return -EINVAL;
	}
	// check inconsistent flags
	if (flags & PAGE_DIR_ENT_4MB){
		return -EINVAL;
	}
	int page_dir_index = GET_DIR_INDEX(virtual_addr);

	// if this page dir entry already exists
	if (page_directory.page_directory_entry[page_dir_index] & (PAGE_DIR_ENT_PRESENT)){
		return -EEXIST;
	}

	// if the page table addr is not aligned to 4KB boundary
	if (((int)new_page_table) % PAGE_4KB){
		return -EINVAL;
	}

	flags |= PAGE_DIR_ENT_PRESENT; // sanity force present flags

	// add the entry in page directory
	page_directory.page_directory_entry[page_dir_index] = ((int)(new_page_table)& 0xFFFFF000) | flags;

	return 0;
}

int page_dir_add_4MB_entry(uint32_t virtual_addr, uint32_t real_addr, int flags){
	// check inconsistent flag
	if (!(flags & PAGE_DIR_ENT_4MB)){
		return -EINVAL;
	}
	// auto fit to nearest 4MB
	int page_dir_index;
	page_dir_index = GET_DIR_INDEX(virtual_addr);

	// if this dir entry already exists
	if (page_directory.page_directory_entry[page_dir_index] & (PAGE_DIR_ENT_PRESENT)){
		return -EEXIST;
	}

	virtual_addr = page_dir_index * PAGE_4MB;
	real_addr = (real_addr / PAGE_4MB) *PAGE_4MB;

	flags |= PAGE_DIR_ENT_PRESENT; // sanity force present flags

	// if want to map to a kernel address, hmmmmmm
	if (page_phys_mem_map[GET_MEM_MAP_INDEX(real_addr)].flags & (PAGE_DES_KERNEL)){
		return -EACCES;
	}

	// if want to map to a physical memory that hasn't been allocated
	if (page_phys_mem_map[GET_MEM_MAP_INDEX(real_addr)].count <= 0){
		return -EINVAL;
	}

	// add the entry in page directory
	page_directory.page_directory_entry[page_dir_index] = (real_addr) | flags;

	return 0;
}

int page_tab_add_entry(uint32_t virtual_addr, uint32_t real_addr, int flags){
	// auto fit to nearest 4KB
	int page_tab_index = GET_TAB_INDEX(virtual_addr);

	// find the page table the virtual address belongs to
	int page_dir_entry_index = GET_DIR_INDEX(virtual_addr);

	real_addr = (real_addr / PAGE_4KB) * PAGE_4KB;

	// check that page table is valid and in use
	if (!(page_directory.page_directory_entry[page_dir_entry_index] & (PAGE_DIR_ENT_PRESENT))){
		return -EINVAL;
	}
	// get the address of the page table
	page_table_t* dest_page_table = (page_table_t*)(page_directory.page_directory_entry[page_dir_entry_index] & 0xFFFFF000);

	// check if already exists
	if (dest_page_table->page_table_entry[page_tab_index] & (PAGE_TAB_ENT_PRESENT)){
		return -EEXIST;
	}

	// if want to map to a kernel address, hmmmmmm
	if (page_phys_mem_map[GET_MEM_MAP_INDEX(real_addr)].flags & (PAGE_DES_KERNEL)){
		return -EACCES;
	}

	// if want to map to a physical memory that hasn't been allocated
	if (manyoushu_mem_table[GET_MANYOU_INDEX(real_addr)].count <= 0){
		return -EINVAL;
	}

	flags |= PAGE_TAB_ENT_PRESENT;	// enforce preset bit

	dest_page_table->page_table_entry[page_tab_index] = (real_addr) | flags;

	return 0;
}

int page_dir_delete_entry(uint32_t virtual_addr){
	// cannot delete dir entries before allocatable ones
	if (GET_DIR_INDEX(virtual_addr) < ALLOCATABLE_4MB_START_INDEX){
		return -EINVAL;
	}

	// if the page dir is not present
	if (page_directory.page_directory_entry[GET_DIR_INDEX(virtual_addr)] & PAGE_DIR_ENT_PRESENT){
		page_directory.page_directory_entry[GET_DIR_INDEX(virtual_addr)] -= PAGE_DIR_ENT_PRESENT;
		return 0;
	}else{
		return -EINVAL;
	}
}

int page_tab_delete_entry(uint32_t virtual_addr){
	// cannot delete page table entries other than manyoushu
	if (GET_DIR_INDEX(virtual_addr) != 3){
		return -EINVAL;
	}

	// check valid page dir entry just for ... redundancy
	if (!(page_directory.page_directory_entry[GET_DIR_INDEX(virtual_addr)] & PAGE_DIR_ENT_PRESENT)){
		return -EINVAL;
	}

	page_table_t* dest_page_table = (page_table_t*)(page_directory.page_directory_entry[GET_DIR_INDEX(virtual_addr)] & (0xFFFFF000));
	// if the page table entry is not present
	if (dest_page_table->page_table_entry[GET_TAB_INDEX(virtual_addr)] & PAGE_TAB_ENT_PRESENT){
		dest_page_table->page_table_entry[GET_TAB_INDEX(virtual_addr)] -= PAGE_TAB_ENT_PRESENT;
		return 0;
	}else{
		return -EINVAL;
	}
}

void page_flush_tlb(){
	__asm__("movl	%cr3, %eax\n\t"
			"movl	%eax, %cr3\n\t");
}


