# x86_desc.S - Set up x86 segment descriptors, descriptor tables
# vim:ts=4 noexpandtab

#define ASM     1
#include "x86_desc.h"

.text

.globl ldt_size, tss_size
.globl gdt_desc, ldt_desc, tss_desc
.globl tss, tss_desc_ptr, ldt, ldt_desc_ptr
.globl gdt_ptr
.globl idt_desc_ptr, idt
# export the page directory starting address
.globl page_directory

.align 4

tss_size:
    .long tss_bottom - tss - 1

ldt_size:
    .long ldt_bottom - ldt - 1

    .word 0 # Padding

gdt_desc:
    .word gdt_bottom - gdt - 1
    .long gdt

ldt_desc:
    .word KERNEL_LDT
    .long ldt

    .align 4
tss:
_tss:
    .rept 104
    .byte 0
    .endr
tss_bottom:

    .align  16
gdt:
_gdt:

    # First GDT entry cannot be used
    .quad 0

    # NULL entry
    .quad 0

    # Segmentation will not be used
    # CS and DS both are 0-4GB r/w segments
    #
    # The layout is (from Intel IA-32 reference manual):
    #  31        24 23  22  21  20  19   16 15  14 13 12  11   8 7          0
    # |----------------------------------------------------------------------|
    # |            |   | D |   | A |  Seg  |   |  D  |   |      |            |
    # | Base 31:24 | G | / | 0 | V | Limit | P |  P  | S | Type | Base 23:16 |
    # |            |   | B |   | L | 19:16 |   |  L  |   |      |            |
    # |----------------------------------------------------------------------|
    #
    # |----------------------------------------------------------------------|
    # |                                    |                                 |
    # | Base 15:0                          | Segment Limit 15:0              |
    # |                                    |                                 |
    # |----------------------------------------------------------------------|

gdt_ptr:
    # Set up an entry for kernel CS
    .quad 0x00CF9A000000FFFF

    # Set up an entry for kernel DS
    .quad 0x00CF92000000FFFF

    # Set up an entry for user CS
    .quad 0x00CFFA000000FFFF

    # Set up an entry for user DS
    .quad 0x00CFF2000000FFFF

    # Set up an entry for TSS
tss_desc_ptr:
    .quad 0

    # Set up one LDT
ldt_desc_ptr:
    .quad 0

gdt_bottom:

    .align 16
ldt:
    .rept 4
    .quad 0
    .endr
ldt_bottom:

.align 4
    .word 0 # Padding
idt_desc_ptr:
    .word idt_bottom - idt - 1
    .long idt


    .align  16
idt:
_idt:
    .rept NUM_VEC
    .quad 0
    .endr

idt_bottom:







# # Page-Directory Entry # #
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
    .long   page_table_0+0b0000000000111 
# Entry - 1 4MB     Set Supervisor bit to kernel address
# bit 0 - Present (P) flag - 1 (enabled)
# bit 1 - R/W - set to 1 (enabled)
# bit 2 - User/supervisor flag - set to 0 (supervisor privilege)
# bit 4:3 - cache flags - currently ignored (0)
# bit 5 - Access flag - set to 0 (not accessed)
# bit 6 - reserved - currently ignored (0)
# bit 7 - page size flag - set to 1 (4 MBytes pages)
# bit 12:8 ignored (0)
    .long   page_table_1+0b0000010000011
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
    .long   0xB8000 + 0b000000000111



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
    .long   0x400000 + 0b000010000011
