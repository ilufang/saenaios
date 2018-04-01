/**
 *	@file page_table.h
 *
 *	page_table initialization and dynamic page allocation(fake)
 */
#ifndef PAGE_TABLE_H_
#define PAGE_TABLE_H_

#include "../lib.h"
#include "../errno.h"
/**
 * 	Initialize page and turn on paging
 *
 *	@note initialize video memory & 4-8MB kernel space!
 */
void page_ece391_init();

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
 *	@param start_addr: start address of the 4MB space that the page 
 *						table is referring to
 *	@param new_page_table: the address of the page table in memory
 *	@param flags: flags of the page directory entry
 *	@note NEED ERROR CHECK for future dynamic situation
 */
int page_dir_add_4KB_entry(int start_addr, void* new_page_table, int flags);

/**
 *	Add a 4MB page table to the page directory
 *
 *	@param start_addr: start address of the 4MB space that the page 
 *						directory entry is referring to
 *	@param flags: flags of the page directory entry
 *	@note NEED ERROR CHECK for future dynamic situation
 */
int page_dir_add_4MB_entry(int virtual_addr, int real_addr, int flags);

/**
 *	Add a 4KB page entry in the page table
 *
 *	@param start_addr: start address of the 4KB space that the page 
 *						table entry is referring to
 *	@param flags: flags of the page table entry
 *	@note NEED ERROR CHECK for future dynamic situation
 */
int page_tab_add_entry(int virtual_addr, int real_addr, int flags);

/**
 *
 *	Flush tlb for changing process
 *
 * 	This would flush all page cache (except global ones)
 * 	
 *	@note flush this much tlb would slower the cpu
 */
void page_flush_tlb();

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
