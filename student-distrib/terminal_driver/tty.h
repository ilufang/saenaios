#ifndef TTY_H_
#define TTY_H_

#include "../keyboard.h"
#include "../error.h"
#include "terminal_out_driver.h"
#include "task.h"
#include "../errno.h"

#define	TTY_SLEEP 			0x0
#define TTY_ACTIVE 			0x1
#define TTY_FOREGROUND 		0x2
#define TTY_BACKGROUND 		0x0

#define TTY_NUMBER			4
#define TTY_BUF_LENGTH		128

#define TTY_FG_BLK		0x0
#define TTY_FG_NBLK		0x1

#define TTY_DRIVER_NAME_LENGTH	32

typedef struct s_tty_buffer{
	uint8_t 	buf[TTY_BUF_LENGTH];
	uint32_t 	index;
	uint32_t 	end;
} tty_buf_t;

typedef struct s_tty{
	uint32_t		tty_status;		///< tty status
	uint32_t 		indev_flag; 	///< flag and feature for input
	uint32_t 		outdev_flag; 	///< flag and feature for output
	uint32_t		flags; 			///< flags and attributes for tty

	s_tty_buffer	buf; 			///< tty buffer

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

void tty_init();

int tty_terminal_init(tty_t* tty);

/**
 *	Exposed tty function for input device drivers to write data into
 *
 *	@param data: buffer of input data
 *	@param size: size of input data length in bytes
 */
void tty_send_input(uint8_t* data, uint32_t size);


int tty_read(file_t* file, uint8_t *buf, size_t count, off_t *offset);

int tty_add_process(task_t* task);

int tty_write();

int tty_open();

int tty_close();

/**
 *	Private function to find index of current foreground tty terminal
 */
int _get_current_tty();

int _tty_switch(tty_t* from, tty_t* to);

int tty_register(uint8_t number, tty_t* new);



#endif
