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
#include "../proc/task.h"

#define VIDEO       0xB8000			///< video memory address start
#define VIDEO_MEM_SIZE	0x1000		///< video memory size
#define NUM_COLS    80				///< 80 characters on one line of the screen
#define NUM_ROWS    25				///< 25 rows on one screen
#define ATTRIB      0x7				///< default attribute for characters on the screen

#define MAX_STDOUT	4 				///< max number of stdout private data
#define MAX_VID_MAP_PAGE 16			///< max number of video map pages

#define TERMINAL_OUT_LF  		10	///< number for control code to new line
#define TERMINAL_OUT_BACKSPACE 	8	///< number for control code to backspace
#define TERMINAL_OUT_FF 		12	///< number for control code to clear screen

/**
 * 	describe a page table entry
 */
typedef struct s_vid_page{
	uint32_t vaddr; ///< Virtual address visible to the process
	uint32_t paddr; ///< Physical address to write into the page table
	uint32_t pt_flags; ///< Flags passed to `page_dir_add_*_entry`
} vid_page_t;

/**
 *	private data of stdout, different data for different tty
 */
typedef struct s_stdout_data{
	int screen_x, screen_y;				///< screen write position
	int cursor_x, cursor_y;				///< screen cursor position
	struct s_vid_page 	vidmem;			///< video memory mapping info
	int 	newline;					///< if newline is present
} stdout_data_t;

/**
 *	Function to write to stdout
 */
ssize_t tty_stdout(uint8_t* data, uint32_t size, void* private_data);


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
 *	@param c: a single character to be printed
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
int terminal_out_write_(uint8_t* buf, uint32_t length);

/**
 *	write for terminal out driver
 *
 *	wrap around of terminal_out_write_
 *
 *	@param file object
 * 	@param buf: A pointer to buffer of output content
 *	@param count: number to be written
 *	@param offset: offset of write
 *	@return number of bytes successfully written
 */
ssize_t terminal_out_write(file_t* file, uint8_t* buf,size_t count,off_t *offset);

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
extern void terminal_set_cursor(void* data);

/**
 *	init a stdout private data for new tty
 */
void* terminal_out_tty_init();

/**
 *	switch video memory when switch tty
 *
 *	@param fromp: data of stdout to be switched out
 *	@param top: data of stdout ot be switched in
 */
int terminal_out_tty_switch(void* fromp, void* top);

/**
 *	map a virtual address to video memory for user
 *
 *	@param start_addr: pointer to the pointer to video memory to be return
 *	@return 0 on success, -1 on failure
 */
int syscall_ece391_vidmap(int start_addr, int b, int c);

#endif
