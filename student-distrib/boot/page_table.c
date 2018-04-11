#include "page_table.h"

static page_directory_t page_directory;

static page_table_t ece391_init_page_table;

void page_ece391_init(){
	// clear the page directory
	memset(&page_directory, 0, 4096);

	// clear the page table of 0-4MB
	memset(&ece391_init_page_table, 0, 4096);

	// create 4KB page for 0-4MB
	page_dir_add_4KB_entry(0x0, (void*)(&ece391_init_page_table), PAGE_DIR_ENT_PRESENT | 
							PAGE_DIR_ENT_RDWR | PAGE_DIR_ENT_SUPERVISOR | 
							PAGE_DIR_ENT_GLOBAL);
	// create 4MB page for 4-8MB
	page_dir_add_4MB_entry(0x400000, 0x400000, PAGE_DIR_ENT_PRESENT | PAGE_DIR_ENT_RDWR | 
							PAGE_DIR_ENT_SUPERVISOR | 
							PAGE_DIR_ENT_GLOBAL);

	// create 4KB page for video memory
	page_tab_add_entry(0xB8000, 0xB8000,PAGE_TAB_ENT_PRESENT | PAGE_TAB_ENT_RDWR |
								PAGE_TAB_ENT_SUPERVISOR | PAGE_TAB_ENT_GLOBAL);
	page_turn_on((int)(&page_directory));
}

int page_dir_add_4KB_entry(int start_addr, void* new_page_table, int flags){
	if (!new_page_table){
		return -EINVAL;
	}
	// NEED ERROR CHECKING!!!!!!!!!!!!!!!!!
	flags -= (flags & PAGE_DIR_ENT_4MB); // sanity force

	page_directory.page_directory_entry[start_addr >> 22] = ((int)(new_page_table)& 0xFFFFF000) | flags;
	return 0;
}

int page_dir_add_4MB_entry(int virtual_addr, int real_addr, int flags){
	// auto fit to nearest 4MB
	int page_dir_index;
	page_dir_index = (virtual_addr / 0x400000);
	virtual_addr = page_dir_index * 0x400000;
	real_addr = (real_addr / 0x400000) *0x400000;
	flags |= PAGE_DIR_ENT_4MB;

	page_directory.page_directory_entry[page_dir_index] = (real_addr) | flags;
	return 0;
}

int page_tab_add_entry(int virtual_addr, int real_addr, int flags){
	// auto fit to nearest 4KB
	int page_tab_index = (virtual_addr / 0x1000);
	
	// find the page table the virtual address belongs to
	int page_dir_entry_index = virtual_addr / 0x400000;

	real_addr = (real_addr / 0x1000) * 0x1000;

	// get the address of the page table
	page_table_t* dest_page_table = (page_table_t*)(page_directory.page_directory_entry[page_dir_entry_index] & 0xFFFFF000);

	// NEED ERROR CHECKING!!!!!!!!!!!
	dest_page_table->page_table_entry[page_tab_index & (0x3FF)] = (real_addr) | flags;

	return 0;
}

void page_flush_tlb(){
	__asm__("movl	%cr3, %eax\n\t"
			"movl	%eax, %cr3\n\t");
}
