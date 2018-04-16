/**
 *	@file proc/signal_user.h
 *
 *	Signal handling support code in user level.
 *
 *	Code defined in signal_user.S will be copied to an address accessible from
 *	user-space programs. These code performs register restoration and other
 *	actions on behalf of the user.
 */
#ifndef PROC_SIGNAL_USER_H
#define PROC_SIGNAL_USER_H

#include "../types.h"

#define PROC_USR_BASE	0x80000000 ///< Base address of this page

/// A dummy value whose address is the start of the code page
extern uint32_t signal_user_base;
/// Size in bytes of the code page
extern size_t signal_user_length;

/**
 *	Return from signal handler.
 *
 *	This function performs register restoration so that code suspended by the
 *	signal can resume normal operation. This function is loaded and called by
 *	`signal.c` and should NOT be called anywhere.
 */
extern void signal_user_ret();
/// Offset of `signal_user_ret` from start of this code page
extern size_t signal_user_ret_offset;
/// Address of `signal_user_ret` when viewed from user-space
#define signal_user_ret_addr ((void *) PROC_USR_BASE + signal_user_ret_offset)

#endif
