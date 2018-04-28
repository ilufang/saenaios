#include "tty.h"

static tty_t tty_list[TTY_NUMBER];[]

static int vidmem_index = 0;

static file_operations_t tty_f_op;

int tty_init(){
	int i; //iterator
	int ret;

	tty_f_op.open = &tty_open;
	tty_f_op.release = &tty_close;
	tty_f_op.read = &tty_read;
	tty_f_op.write = &tty_write;
	tty_f_op.llseek = NULL;
	tty_f_op.readdir = NULL;
	// register tty driver
	ret = devfs_register_driver("tty", &tty_f_op);
	if (ret) return ret;
	// initialize all terminals to sleep
	for (i=0; i<TTY_NUMBER; ++i){
		tty_list[i].tty_status = TTY_SLEEP;
	}
	// initialize the first tty
	tty_list[0].tty_status = TTY_ACTIVE | TTY_FOREGROUND;

	// open stdin & stdout
	tty_list[0].output_private_data = terminal_out_tty_init();

	return 0;
}

int tty_switch(int to_index){
	int from_index = _get_current_tty();
	if (tty_list[to_index].tty_status == TTY_SLEEP){
		_tty_init(to_index);
	}
	_tty_switch(tty_list[from_index],tty_list[to_index]);
}

int _single_tty_init(int index){
	// initialize flags, 0 for now
	tty->indev_flag = 0;
	tty->outdev_flag = 0;
	tty->flags = 0;

	// initialize buffer
	tty->buf.index = 0;
	tty->buf.end = TTY_BUF_LENGTH - 1;

	// default input is stdin, default output is stdout
	// stdin is universal, so no private data for it
	tty->input_private_data = NULL;
	// initialize another stdout instance
	tty->output_private_data = terminal_out_tty_init();
	if (!tty->output_private_data){
		// error
		return -errno;
	}

	tty_status |= TTY_ACTIVE;
}

int _tty_switch(tty_t* from, tty_t* to){
	// change tty structure status
	from -> status &= ~TTY_FOREGROUND;
	to -> status |= TTY_FOREGROUND;

	// switch private data, let terminal out driver handle it
	return terminal_out_tty_switch(from->output_private_data, to->input_private_data);
}

int _get_current_tty(){
	int i;

	for (i=0; i<TTY_NUMBER; ++i){
		if (tty_list[i].tty_status & TTY_FOREGROUND){
			// found the foreground tty
			return i;
		}
	}
	return -1;	// very bad thing happened
}

int tty_open(struct s_inode *inode, struct s_file *file){
	// do nothing
	return 0;
}

int tty_close(struct s_inode *inode, struct s_file *file){
	// do nothing
	return 0;
}

ssize_t tty_read(struct s_file *file, uint8_t *buf, size_t count, off_t *offset){
	// blocking read from tty buffer
}

ssize_t (*write)(struct s_file *file, uint8_t *buf, size_t count, off_t *offset){
	// write something to stdout
	return tty_stdout(buf, count, tty_list[_get_current_tty].output_private_data);
}
