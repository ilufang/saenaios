#define ASM     1
# export the page directory starting address
.text

.globl page_init
# Page-Directory Entry # #
# 31 - 12: Page-Table Base Address 
# 11 - 9 : Avail: Available for system programmer's use
# 8      : Global Page (ignored)
# 7      : Page Size (0 indicates 4KBytes)
# 6      : Reserved (set to 0)
# 5      : Accessed
# 4      : Cache disabled
# 3      : Write - through 
# 2      : User/Supervisor
# 1      : Read/Write
# 0      : Present

.align  4096
page_directory:

# Entry - 0 4KB
# bit 0 - Present (P) flag - 1 (enabled)
# bit 1 - R/W - set to 1 (enabled)
# bit 2 - User/supervisor flag - set to 1 (User privilege)
# bit 4:3 - cache flags - currently ignored (0)
# bit 5 - Access flag - set to 0 (not accessed)
# bit 6 - reserved - currently ignored (0)
# bit 7 - page size flag - set to 0 (4 KBytes pages)
# bit 12:8 ignored (0)

    .long   page_table_0 + 0x007   // 0b0000000000111 

# Entry - 1 4MB     Set Supervisor bit to kernel address
# bit 0 - Present (P) flag - 1 (enabled)
# bit 1 - R/W - set to 1 (enabled)
# bit 2 - User/supervisor flag - set to 0 (supervisor privilege)
# bit 4:3 - cache flags - currently ignored (0)
# bit 5 - Access flag - set to 0 (not accessed)
# bit 6 - reserved - currently ignored (0)
# bit 7 - page size flag - set to 1 (4 MBytes pages)
# bit 12:8 ignored (0)

    .long   page_table_1 + 0x083// 0b0000010000011
# Reserved TODO


# # Page-Table Entry 4KB Page # #
# 31 - 12: Page-Table Base Address 
# 11 - 9 : Avail: Available for system programmer's use
# 8      : Global Page
# 7      : Page Table Attribute Index
# 6      : Dirty
# 5      : Accessed
# 4      : Cache disabled
# 3      : Write - through 
# 2      : User/Supervisor
# 1      : Read/Write
# 0      : Present

.align  4096
page_table_0:

# Page-Table 0 4KB
# Entry 0 Video Memory fixed at 0xB8000
# bit 0 - Present (P) flag - 1 (enabled)
# bit 1 - R/W - set to 1 (enabled)
# bit 2 - User/supervisor flag - set to 1 (User privilege)
# bit 4:3 - cache flags - currently ignored (0)
# bit 5 - Access flag - set to 0 (not accessed)
# bit 6 - Dirty - set to 0 (not written)
# bit 7 - page attribute table index - set to 0 (4 KBytes pages)
# bit 12:8 ignored (0)

    .rept   0x2E0
    .byte   0x0
    .endr
    .long   0xB8000 + 0x003    // 0b000000000011 

# # Page-Table Entry 4MB Page # #
# 31 - 22: Page-Table Base Address 
# 21 - 13: Reserved
# 11 - 9 : Avail: Available for system programmer's use
# 8      : Global Page
# 7      : Page Table Attribute Index
# 6      : Dirty
# 5      : Accessed
# 4      : Cache disabled
# 3      : Write - through 
# 2      : User/Supervisor
# 1      : Read/Write
# 0      : Present


.align 4096
page_table_1:

# Page-Table 1 4MB - Map to kernel
# Only 1 Entry here
# bit 0 - Present (P) flag - 1 (enabled)
# bit 1 - R/W - set to 1 (enabled)
# bit 2 - User/supervisor flag - set to 1 (supervisor privilege)
# bit 4:3 - cache flags - currently ignored (0)
# bit 5 - Access flag - set to 0 (not accessed)
# bit 6 - Dirty - set to 0 (not written)
# bit 7 - page side - set to 0 (4 KBytes pages)
# bit 12:8 ignored (0)
# bit 21:13 Reserved (0)

    .long   0x400000 + 0x083    //0b000010000011

# page init function
#   Input: None
#   Output: None
#   Return Value: None
#   Side Effect: Initialize paging and turn on paging
page_init:
	pushl	%ebp
	movl	%esp, %ebp

	# Load the Page Directory 
    movl    $page_directory, %eax
    movl    %eax, %cr3
    # enable 4KB 4MB mixing pages by setting PSE flag, bit 4 of CR4
    movl    %cr4, %eax
    orl     $0x00000010, %eax
    movl    %eax, %cr4

    # enable paging by modifying PG flag, 31 bit of CR0
    movl    %cr0, %eax
    orl     $0x80000000, %eax
    movl    %eax, %cr0

    leave
    ret
