/**
 *	@file ece391_syscall.h
 *
 *	ece391 system calls
 */
#ifndef ECE391_SYSCALL_H
#define ECE391_SYSCALL_H


#include "../lib.h"
#include "../fs/vfs.h"
#include "../fsdriver/mp3fs_driver.h"
#include "../proc/task.h"
#include "../../libc/src/syscalls.h"
#include "../x86_desc.h"
#include "page_table.h"
#include "../libc.h"
#include "syscall.h"

// why is there a 48000 offset in discussion slide though
#define PROG_IMG_OFFSET 0x8048000   ///< starting point to load an program image
#define	RPL_USR			0x03		///< bitmask offset for privilege level 3
#define ENTRY_POINT_OFFSET	24		///< start of entry point address in elf file
#define PHYS_MEM_OFFSET	0xC00000	///< starting offset of physical memory.
#define VID_MEM_OFFSET	0xB8000		///< physical address of video memory
#define VID_VIRT_OFFSET 0x8400000	///< pre-defined virtual address of video memory


/**
 *	Execute syscall handler
 *
 *	@param command: the command to execute
 *	@return -1 on execution error or value returned from the program
 */
int32_t syscall_ece391_execute(int command, int, int);

/**
 *	Halt syscall handler
 *
 *	@return the return value from the halted process
 */
int32_t syscall_ece391_halt(int status, int, int);

/**
 *	Getargs syscall handler
 *
 *	@param buf: buffer to copy arguments
 *	@param nbytes: number of bytes to copy
 *	@return -1 on error, 0 on success
 */
int32_t syscall_ece391_getargs(int buf, int nbytes, int);

/**
 *	Vidmap syscall handler
 *
 *	@param screen_start_in: user pointer that stores the virtual address
 *	@return -1 on error, virtual address of video memory on success
 */
int32_t syscall_ece391_vidmap(int screen_start_in, int, int);

#endif
