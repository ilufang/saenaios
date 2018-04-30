#ifndef TTY_H_
#define TTY_H_

#include "../errno.h"
#include "terminal_out_driver.h"
#include "../proc/task.h"
#include "../errno.h"
#include "../proc/signal.h"

#define	TTY_SLEEP 			0x0 		///< tty flag, means tty is not in use
#define TTY_ACTIVE 			0x1			///< tty flag, means tty is in use
#define TTY_FOREGROUND 		0x2 		///< tty flag, means tty is foreground
#define TTY_BACKGROUND 		0x0 		///< tty flag, means tty is background

#define TTY_NUMBER			4 			///< maximum number of tty
#define TTY_BUF_LENGTH		128 		///< maximum length of tty buffer

#define TTY_FG_ECHO		 	0x0 		///< tty echo any key press to the terminal
#define TTY_FG_NECHO		0x1 		///< tty doesn't echo key press

#define TTY_BUF_ENTER		0x1 		///< buffer flag flag for there is an enter in the buffer

#define TTY_DRIVER_NAME_LENGTH	32 		///< maximum length of in/out driver name length

/**
 *	tty buffer structure stores all information about the buffer of a tty
 * 	including flags, current index, end of buffer, and the buffer data,
 * 	note that the buffer is a wrapped around structure
 */
typedef struct s_tty_buffer{
	uint8_t 	buf[TTY_BUF_LENGTH]; 	///< buffer data
	uint8_t 	flags; 					///< buffer flag
	uint32_t 	index; 					///< index of next empty spot
	uint32_t 	end; 					///< end of the buffer
} tty_buf_t;

/**
 *	tty structure stores all the information of a tty
 */
typedef struct s_tty{
	uint32_t		tty_status;		///< tty status
	uint32_t 		indev_flag; 	///< flag and feature for input
	uint32_t 		outdev_flag; 	///< flag and feature for output
	uint32_t		flags; 			///< flags and attributes for tty
	uint32_t 		fg_proc;		///< process pid currently running at foreground of this tty
	uint32_t 		root_proc;		///< first process born with this tty
	struct s_tty_buffer	buf; 			///< tty buffer

	uint32_t 		input_pid_waiting; 		///< pid of the process waiting for input, 0 for none
	void* 			input_private_data; 	///< input private data, memory allocated by input driver
	void* 			output_private_data;	///< output private data, memory allocated by output driver

	// not implemented
/*	int (*tty_write)(uint8_t* buf, uint32_t* size); ///< output function pointer
	int (*tty_in_open)(struct s_tty* tty); 	///< open input driver
	int (*tty_out_open)(struct s_tty* tty); ///< open output driver*/
} tty_t;

/// Current active TTY
extern tty_t* cur_tty;

// not implemented
/*typedef struct s_tty_in_driver{
	char	name[TTY_DRIVER_NAME_LENGTH];
	int 	(*open)(struct s_tty* tty);
} tty_in_driver_t;

typedef struct s_tty_out_driver{
	char	name[TTY_DRIVER_NAME_LENGTH];
	int 	(*open)(struct s_tty* tty);
} tty_out_driver_t;*/

/**
 *	initialize tty part, initialize all ttys, activate the first one
 *
 *	@return 0 on success
 */
int tty_init();

/**
 *	private function to fork out a process, and set it to run ece391 shell
 *
 *	return the pid of forked process on success, negative error codes on error
 */
int _tty_start_shell();

/**
 *	function to register tty driver to devfs, initialize file operation table
 *
 *	return 0 on success, negative error codes on error
 */
int tty_driver_register();

/**
 *	private function to activate a tty, and create its root process as a shell
 *
 *	return 0 on success, negative error codes on error
 */
int _single_tty_init(int index);

/**
 *	Exposed tty function for input device drivers to write data into
 *
 *	@param data: buffer of input data
 *	@param size: size of input data length in bytes
 */
void tty_send_input(uint8_t* data, uint32_t size);

/**
 *	attach a process to tty
 *
 * 	@note will create new stdin stdout file if new tty
 */
void tty_attach(task_t* proc);

/**
 *	detach a process from its tty
 *
 *	@note not used
 */
void tty_detach(task_t* proc);

/**
 *	Read function for tty
 *
 *	@param file: the file to read
 *	@param buf: the buffer to read into
 *	@param count: maximum number of bytes, or size of buffer
 *	@param offset: pointer to file pointer. This function must use this
 *				   parameter, not the `pos` field in `file`. Although it
 *				   will point to `pos` most of the time, VFS may specify
 *				   a different value and must preserve `pos`, such as in
 *				   `pread`
 *	@return number of bytes read, or the negative of an errno on failure
 */
ssize_t tty_read(struct s_file *file, uint8_t *buf, size_t count, off_t *offset);

/**
 *	Write function for tty
 *
 *	@param file: the file to write
 *	@param buf: the data to write
 *	@param count: the size of buffer
 *	@param offset: see `offset` of `read`
 *	@return number of bytes written, or the negative of an errno on failure
 */
ssize_t tty_write(struct s_file *file, uint8_t *buf, size_t count, off_t *offset);

/**
 *	Open function for tty
 *
 *	private data written for tty index
 *
 *	@param inode: the i-node the file is opened from
 *	@param file: the newly-opened file
 *	@return 0 on success, or the negative of an errno on failure
 */
int tty_open(struct s_inode *inode, struct s_file *file);

/**
 *	Release function for tty
 *
 *	@param inode: the i-node the file is opened from
 *	@param file: the file to be closed
 *	@return 0 on success, or the negative of an errno on failure
 */
int tty_close(struct s_inode *inode, struct s_file *file);

/**
 *	Private function to find index of current foreground tty terminal
 */
uint8_t get_current_tty();

/**
 * 	external call to switch to another tty
 *
 * 	@param to_index: the index of the tty to switch to
 *	@return 0 on success, negative error number on errors
 */
int tty_switch(int to_index);

/**
 *	private helper function to switch tty
 *
 *	@param from: the tty to be switched off
 *	@param to: the tty to be switched on
 *	@return 0 on success, negative error number on errors
 */
int _tty_switch(tty_t* from, tty_t* to);

#endif
