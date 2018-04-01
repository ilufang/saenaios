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
#include "../proc/pcb.h"
#include "../../libc/src/syscalls.h"
#include "../x86_desc.h"
#include "page_table.h"

// why is there a 48000 offset in discussion slide though
#define PROG_IMG_OFFSET 0x8048000   ///< starting point to load an program image
#define	RPL_USR			0x03		///< bitmask offset for privilege level 3
#define SW_USR_CS		0x0023
#define SW_USR_DS		0x002B
#define SW_STACK		0x83FFFFC	///< stack start pointer at top of 4MB page

int32_t syscall_ece391_execute(const uint8_t* command);

int32_t syscall_ece391_halt(uint8_t stauts);


#endif
