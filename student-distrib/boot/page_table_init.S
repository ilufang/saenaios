#define ASM     1
# export the page directory starting address
.global page_turn_on
#   page init function
#   Input: None
#   Output: None
#   Return Value: None
#   Side Effect: Initialize paging and turn on paging
page_turn_on:
	pushl	%ebp
	movl	%esp, %ebp

	# Load the Page Directory
    movl    8(%ebp), %eax
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
