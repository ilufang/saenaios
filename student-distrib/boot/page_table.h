/**
 *	@file page_table.h
 *
 *	page_table initialization and dynamic page allocation(fake)
 */
#ifndef PAGE_TABLE_H_
#define PAGE_TABLE_H_

#include "../lib.h"
#include "../errno.h"

#define MAX_DYNAMIC_4MB_PAGE 63

/**
 * 	Initialize page, initialize physical memory map, and turn on paging
 *
 *	@note initialize video memory & 4-8MB kernel space, reserve 8-CMB for usage,
 * 			and also C-10 MB for 4KB Manyoushu
 */
void page_ece391_init();

void page_kernel_mem_map_init();

void page_phys_mem_map_init();

/**
 *	Turn on paging in the processor
 *
 *	@param page_directory: address of page directory
 *	
 *	@note the address should be aligned to 4KB on memory
 */
extern void page_turn_on(int page_directory);

/**
 *	Add a 4KB page table to the page directory
 *
 *	@param virtual_addr: start address of the 4MB space that the page 
 *						table is referring to
 *	@param new_page_table: the address of the page table in memory
 *	@param flags: flags of the page directory entry
 *	@return: 0 on success, negative value for errors
 *	@note Will be rejected if the entry already exists
 */
int page_dir_add_4KB_entry(int virtual_addr, void* new_page_table, int flags);

/**
 *	Add a 4MB page table to the page directory
 *
 *	@param virtual_addr: start address of the 4MB space that the page 
 *						directory entry is referring to
 *	@param real_addr: physical memory that is referring to, must be a allocated one
 *	@param flags: flags of the page directory entry
 *	@return: 0 on success, negative value for errors
 *	@note Will be rejected if the entry already exists
 */
int page_dir_add_4MB_entry(int virtual_addr, int real_addr, int flags);

/**
 *	Add a 4KB page entry in the page table
 *
 *	@param start_addr: start address of the 4KB space that the page 
 *						table entry is referring to
 *	@param real_addr: physical memory that is referring to, must be a allocated one
 *	@param flags: flags of the page table entry
 *	@return: 0 on success, negative value for errors
 *	@note Will be rejected if the entry already exists
 */
int page_tab_add_entry(int virtual_addr, int real_addr, int flags);

/**
 *	Free a 4MB in the page directory
 *
 *	@param virtual_addr: virtual address to be freed
 *	@return: 0 on success, negative value for errors
 *	@note Will be rejected if want to free a kernel entry
 */
int page_dir_delete_entry(int virtual_addr);

/**
 *	Free a 4KB page entry to the page table
 *
 *	@param virtual_addr: virtual address to be freed
 *	@return: 0 on success, negative value for errors
 *	@note Will be rejected if want to free a kernel entry
 */
int page_tab_delete_entry(int virtual_addr);
/**
 *
 *	Flush tlb for changing process
 *
 * 	This would flush all page cache (except global ones)
 * 	
 *	@note flush this much tlb would slower the cpu
 */
void page_flush_tlb();

/**
 *
 *	private function to find a usable 4MB page
 *
 *	@return positive value for a address, negative value for error
 *
 *	@note the sign of return value
 */
int _page_alloc_get_4MB();

/**
 *
 *	private function to find a usable 4KB page
 *
 *	@return positive value for a address, negative value for error
 *
 *	@note the sign of return value may restrict the memory space
 */
int _page_alloc_get_4KB();

/**
 *	
 *	private function to add reference count to a 4MB page
 *
 *	@oaram addr: the physical address to add reference
 *	@return 0 for success, negative value for error
 */
int _page_alloc_add_refer_4MB(int addr);

/**
 *	
 *	private function to add reference count to a 4KB page
 *
 *	@oaram addr: the physical address to add reference
 *	@return 0 for success, negative value for error
 */
int _page_alloc_add_refer_4KB(int addr);

/**
 *
 *	exposed function to allocate or reference a 4MB page 
 *
 *	@param physical_addr: if the pointer points to value 0, then allocate a space,
 *	and change the pointed value; if the pointer points to a non-zero value, then add
 *	count to the physical memory referenced
 *
 *	@return 0 for success, negative value for error
 */
int page_alloc_4MB(int* physical_addr);

/**
 *
 *	exposed function to allocate or reference a 4KB page 
 *
 *	@param physical_addr: if the pointer points to value 0, then allocate a space,
 *	and change the pointed value; if the pointer points to a non-zero value, then add
 *	count to the physical memory referenced
 *
 *	@return 0 for success, negative value for error
 */
int page_alloc_4KB(int* physical_addr);

/**
 *
 *	exposed function to free a 4MB page 
 *
 *	@param physical_addr: value of the physical address to free
 *
 *	@return 0 for success, negative value for error
 *
 *	@note actually decrease the use count of that memory
 */
int page_alloc_free_4MB(int physical_addr);

/**
 *
 *	exposed function to free a 4KB page 
 *
 *	@param physical_addr: value of the physical address to free
 *
 *	@return 0 for success, negative value for error
 *
 *	@note actually decrease the use count of that memory
 */
int page_alloc_free_4KB(int physical_addr);

typedef int page_directory_entry_t;		///< page directory entry

typedef int page_table_entry_t;		///< page table entry

/**
 *	page directory struct, need to be packed, and aligned to 4KB memory
 */
typedef struct s_page_directory{
	page_directory_entry_t page_directory_entry[1024];	///< page directory entry array, must be 1024
} __attribute__((packed, aligned(4096))) page_directory_t;

/**
 *	page table struct, need to be packed, and aligned to 4KB memory
 */
typedef struct s_page_table{
	page_table_entry_t page_table_entry[1024]; ///< page table entry array, must be 1024
} __attribute__((packed, aligned(4096))) page_table_t;

typedef struct s_page_4MB_descriptor{
	uint32_t				flags;	///< private flags for physical memory administration
	int 					count;	///< use count or reference count
	struct s_page_4KB_descriptor* 	pages;
} page_4MB_descriptor_t;

typedef struct s_page_4KB_descriptor{
	uint32_t 	flags;		///< private flags for physical memory administration
	int 		count;		///< use count or reference count
} page_4KB_descriptor_t;

/*
 *	flags for physical memory map
 */
#define PAGE_DES_KERNEL		0x02	///< dadada
#define	PAGE_DES_RESERVE	0x04	///< dadada
/*
 *	4KB page directory entry
 *	
 *	bit 0 - Present (P) flag - 1 (enabled)
 *	bit 1 - R/W - set to 1 (enabled)
 *	bit 2 - User/supervisor flag - set to 1 (User privilege)
 *	bit 4:3 - cache flags - currently ignored (0)
 *	bit 5 - Access flag - set to 0 (not accessed)
 *	bit 6 - reserved - currently ignored (0)
 *	bit 7 - page size flag - set to 0 (4 KBytes pages)
 *	bit 12:8 ignored (0)
 */
/*
 *	4MB page directory entry
 *	
 *	bit 0 - Present (P) flag - 1 (enabled)
 *	bit 1 - R/W - set to 1 (enabled)
 *	bit 2 - User/supervisor flag - set to 0 (supervisor privilege)
 *	bit 4:3 - cache flags - currently ignored (0)
 *	bit 5 - Access flag - set to 0 (not accessed)
 *	bit 6 - reserved - currently ignored (0)
 *	bit 7 - page size flag - set to 1 (4 MBytes pages)
 *	bit 12:8 ignored (0)
 */
/*
 *	Page-Table Entry 4KB Page
 *	31 - 12: Page-Table Base Address
 *	11 - 9 : Avail: Available for system programmer's use
 *	8      : Global Page
 *	7      : Page Table Attribute Index
 *	6      : Dirty
 *	5      : Accessed
 *	4      : Cache disabled
 *	3      : Write - through
 *	2      : User/Supervisor
 *	1      : Read/Write
 *	0      : Present
 */
#define PAGE_DIR_ENT_PRESENT			0x01	///<flag, as name suggested
#define PAGE_DIR_ENT_NPRESENT			0X00	///<flag, as name suggested

#define PAGE_DIR_ENT_RDONLY				0x00	///<flag, as name suggested
#define PAGE_DIR_ENT_RDWR				0x02	///<flag, as name suggested

#define	PAGE_DIR_ENT_USER				0x04	///<flag, as name suggested
#define PAGE_DIR_ENT_SUPERVISOR			0x00	///<flag, as name suggested

#define PAGE_DIR_ENT_4MB				0x80	///<flag, as name suggested
#define PAGE_DIR_ENT_4KB				0X00	///<flag, as name suggested

#define PAGE_DIR_ENT_GLOBAL				0x100	///<flag, as name suggested

#define PAGE_TAB_ENT_PRESENT			0x01	///<flag, as name suggested
#define PAGE_TAB_ENT_NPRESENT			0x00	///<flag, as name suggested

#define PAGE_TAB_ENT_RDONLY				0x00	///<flag, as name suggested
#define PAGE_TAB_ENT_RDWR				0x02	///<flag, as name suggested

#define	PAGE_TAB_ENT_USER				0x04	///<flag, as name suggested
#define PAGE_TAB_ENT_SUPERVISOR			0x00	///<flag, as name suggested

#define PAGE_TAB_ENT_GLOBAL				0x100	///<flag, as name suggested

#endif
