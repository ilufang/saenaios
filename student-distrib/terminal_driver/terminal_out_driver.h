/**
 *	@file terminal_out_driver.h
 *
 *	Header file for terminal output driver
 *
 */


#ifndef _TERMINAL_DRIVER_H
#define _TERMINAL_DRIVER_H

#include "../types.h"
#include "../lib.h"
#include "../fs/vfs.h"
#include "../fs/fs_devfs.h"

#define VIDEO       0xB8000			///< video memory address start
#define VIDEO_MEM_SIZE	0x1000		///< video memory size
#define NUM_COLS    80				///< 80 characters on one line of the screen
#define NUM_ROWS    25				///< 25 rows on one screen
#define ATTRIB      0x7				///< default attribute for characters on the screen

/**
 *	Perform newline
 *
 *	This function creates a newline effect on the screen, not called when line
 *	exceeds terminal width
 *
 *	@note after this function is called, backspace would not be able to return to last row
 */
void terminal_out_newline();

/**
 *	Perform backspace
 *
 *	This function creates a backspace effect on the screen.
 *
 *	@note cannot backspace to previous line if it is newline/clear screen before
 */
void terminal_out_backspace();

/**
 *	Handle escape sequence
 *
 *	This function handles escape sequence and dispatch special handlers
 *
 *  @param buf: buffer of the escape sequence to be handled, 
 *				buffer may be trailed with remaining characters to be printed
 *  @param max_length: length that tells how many characters are left in the buffer
 *	@return the length of the escape sequence handled
 */
int terminal_out_escape_sequence(uint8_t *buf,int max_length);

/**
 *	Perform clear screen
 *
 *	This function creates a clear screen effect on the screen.
 *
 *	@note cannot backspace
 */
void terminal_out_clear();

/**
 *	Print a single character on the terminal 
 *
 *	This function prints a single character on the screen
 *
 *	@param a single character to be printed
 */
void terminal_out_putc(uint8_t c);

/**
 *	Perform terminal screen scroll down
 *
 *	This function scroll down the screen by one line
 *
 *	@note don't expect to hit backspace, and then scroll up.
 *		And this function performs memory operation almost the size of video memory
 *		So, maybe slow.
 */
void terminal_out_scroll_down();

/**
 *	open for terminal out driver
 *
 *	actually does nothing for now, but need to change later
 *
 *	@param inode object
 * 	@param file object
 *	@return 0 on success, errno for errors
 */
int terminal_out_open(inode_t* inode, file_t* file);

/**
 *	close for terminal out driver
 *
 *	actually does nothing for now, but need to change later
 *
 *	@param inode object
 * 	@param file object
 *	@return 0 on success, errno for errors
 */
int terminal_out_close(inode_t* inode,file_t* file);

/**
 *	The only function exposed to write to the terminal
 *
 *	This function would be used in generic call write later.
 *  The output driver would recognize '\n', escape sequence
 *
 *	@param buf: A pointer to buffer of output content
 *	@param length: size of buffer
 * 	@return number of bytes successfully written
 */
int terminal_out_write_(uint8_t* buf, int length);

/**
 *	write for terminal out driver
 *
 *	wrap around of terminal_out_write_
 *
 *	@param file object
 * 	@param buf: A pointer to buffer of output content
 *	@param count: number to be written
 *	@return number of bytes successfully written
 */
ssize_t terminal_out_write(file_t* file, uint8_t* buf,size_t count,off_t *offset);

/**
 *	read for terminal out driver
 *
 *	this is invalid operation of terminal out dirver
 *
 *	@param file object
 * 	@param buf: A pointer to buffer of output content
 *	@param count: number to be read
 *	@return 0
 *	
 *  @note behavior not defined!
 */
ssize_t terminal_out_read(file_t* file, uint8_t *buf, size_t count, off_t *offset);
/**
 *	register the terminal out driver
 *	
 *	or stdout, which I may call it
 */
int terminal_out_driver_register();

/**
 *
 *	set cursor position
 *
 *	set cursor position according to current position on the screen
 *	@note this is a private function!
 */
void terminal_set_cursor();

#endif
