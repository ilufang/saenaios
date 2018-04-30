#ifndef TTY_H_
#define TTY_H_

#include "../errno.h"
#include "terminal_out_driver.h"
#include "../proc/task.h"
#include "../errno.h"
#include "../proc/signal.h"

#define	TTY_SLEEP 			0x0
#define TTY_ACTIVE 			0x1
#define TTY_FOREGROUND 		0x2
#define TTY_BACKGROUND 		0x0

#define TTY_NUMBER			4
#define TTY_BUF_LENGTH		128

#define TTY_FG_ECHO		 	0x0
#define TTY_FG_NECHO		0x1

#define TTY_BUF_ENTER		0x1

#define TTY_DRIVER_NAME_LENGTH	32

typedef struct s_tty_buffer{
	uint8_t 	buf[TTY_BUF_LENGTH];
	uint8_t 	flags;
	uint32_t 	index;
	uint32_t 	end;
} tty_buf_t;

typedef struct s_tty{
	uint32_t		tty_status;		///< tty status
	uint32_t 		indev_flag; 	///< flag and feature for input
	uint32_t 		outdev_flag; 	///< flag and feature for output
	uint32_t		flags; 			///< flags and attributes for tty

	struct s_tty_buffer	buf; 			///< tty buffer

	uint32_t 		input_pid_waiting;
	void* 			input_private_data; 	///< input private data, memory allocated by input driver
	void* 			output_private_data;	///< output private data, memory allocated by output driver

	// not implemented
/*	int (*tty_write)(uint8_t* buf, uint32_t* size); ///< output function pointer
	int (*tty_in_open)(struct s_tty* tty); 	///< open input driver
	int (*tty_out_open)(struct s_tty* tty); ///< open output driver*/
} tty_t;

// not implemented
/*typedef struct s_tty_in_driver{
	char	name[TTY_DRIVER_NAME_LENGTH];
	int 	(*open)(struct s_tty* tty);
} tty_in_driver_t;

typedef struct s_tty_out_driver{
	char	name[TTY_DRIVER_NAME_LENGTH];
	int 	(*open)(struct s_tty* tty);
} tty_out_driver_t;*/

int tty_init();

int tty_driver_register();

int _single_tty_init(int index);

/**
 *	Exposed tty function for input device drivers to write data into
 *
 *	@param data: buffer of input data
 *	@param size: size of input data length in bytes
 */
void tty_send_input(uint8_t* data, uint32_t size);


ssize_t tty_read(struct s_file *file, uint8_t *buf, size_t count, off_t *offset);

ssize_t tty_write(struct s_file *file, uint8_t *buf, size_t count, off_t *offset);

int tty_open(struct s_inode *inode, struct s_file *file);

int tty_close(struct s_inode *inode, struct s_file *file);

/**
 *	Private function to find index of current foreground tty terminal
 */
uint8_t get_current_tty();

int tty_switch(int to_index);

int _tty_switch(tty_t* from, tty_t* to);

int tty_register(uint8_t number, tty_t* new);



#endif
