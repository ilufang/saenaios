#include "tty.h"
#include "../lib.h"
static tty_t tty_list[TTY_NUMBER];

static file_operations_t tty_f_op;

static char shell_cmdline[6] = "shell";
static char* argv_placeholder = NULL;

tty_t* cur_tty = NULL;
static uint8_t temp_buf[TTY_BUF_LENGTH];

//static uint32_t keyboard_pid_waiting = 0;

int tty_init(){
	int i; //iterator

	// set the f**king libc
	_set_tty_start_();

	// initialize all terminals to sleep
	for (i=0; i<TTY_NUMBER; ++i){
		tty_list[i].tty_status = TTY_SLEEP | TTY_BACKGROUND;
		// initialize flags, 0 for now
		tty_list[i].indev_flag = 0;
		tty_list[i].outdev_flag = 0;
		tty_list[i].flags = 0;
		tty_list[i].input_pid_waiting = 0;

		// initialize buffer
		tty_list[i].buf.index = 0;
		tty_list[i].buf.end = TTY_BUF_LENGTH - 1;
		tty_list[i].buf.flags = 0;
	}
	// initialize the first tty
	tty_list[0].tty_status = TTY_ACTIVE | TTY_FOREGROUND;
	tty_list[0].output_private_data = terminal_out_tty_init();
	tty_list[0].fg_proc = 1; 		// NOTE HARDCODED
	tty_list[0].root_proc = 1;		// NOTE HARDCODED

	return 0;
}

int tty_driver_register(){
	tty_f_op.open = &tty_open;
	tty_f_op.release = &tty_close;
	tty_f_op.read = &tty_read;
	tty_f_op.write = &tty_write;
	tty_f_op.llseek = NULL;
	tty_f_op.readdir = NULL;
	// register tty driver
	return devfs_register_driver("tty", &tty_f_op);
}

int tty_switch(int to_index){
	int from_index = get_current_tty();
	if (tty_list[to_index].tty_status == TTY_SLEEP){
		_single_tty_init(to_index);
		int child_pid = _tty_start_shell();
		if (child_pid < 0)	return child_pid;

		tty_list[to_index].fg_proc = child_pid;
		tty_list[to_index].root_proc = child_pid;
	}
	if (from_index == to_index) return 0;
	return  _tty_switch(&tty_list[from_index],&tty_list[to_index]);
}

int _tty_start_shell(){
	// create new process of fork
	int child_pid = syscall_fork(0,0,0);
	if (child_pid < 0){
		//error condition
		return child_pid;
	}
	task_t* child_proc = task_list + child_pid;
	child_proc->regs.eax = SYSCALL_EXECVE;
	child_proc->regs.ebx = (uint32_t)shell_cmdline;
	child_proc->regs.ecx = (uint32_t)argv_placeholder;
	child_proc->regs.edx = 0;
	child_proc->regs.eip = syscall_ece391_execute_magic + 0x8000000;

	return child_pid;
}

int _single_tty_init(int index){
	tty_t* tty = &tty_list[index];
	// default input is stdin, default output is stdout
	// stdin is universal, so no private data for it
	tty->input_private_data = NULL;
	// initialize another stdout instance
	tty->output_private_data = terminal_out_tty_init();
	if (!tty->output_private_data){
		// error
		return -errno;
	}

	tty->tty_status |= TTY_ACTIVE;
	return 0;
}

int _tty_switch(tty_t* from, tty_t* to){
	// change tty structure status
	from->tty_status &= ~TTY_FOREGROUND;
	to->tty_status |= TTY_FOREGROUND;

	cur_tty = to;

	// switch private data, let terminal out driver handle it
	return terminal_out_tty_switch(from->output_private_data, to->output_private_data);
}

void tty_attach(task_t* proc){
	if (!cur_tty) {
		cur_tty = &tty_list[0];
	}
	proc->tty = get_current_tty();
	tty_list[proc->tty].fg_proc = proc->pid;
	// if this is a root process of a tty, and it differs with the files
	if ((proc->files[1]->private_data != proc->tty)
		&& (proc->pid == cur_tty->root_proc)){
		// means this process should open new in/out
		syscall_close(0,0,0);
		syscall_close(1,0,0);
		syscall_close(2,0,0);
		syscall_open((int)"/dev/stdin", O_RDONLY, 0);
		syscall_open((int)"/dev/stdout", O_WRONLY, 0);
		syscall_open((int)"/dev/stderr", O_WRONLY, 0);
	}
}

void tty_detach(task_t* proc){
	tty_list[proc->tty].fg_proc = proc->parent;
}

uint8_t get_current_tty(){
	int i;

	for (i=0; i<TTY_NUMBER; ++i){
		if (tty_list[i].tty_status & TTY_FOREGROUND){
			return i;
		}
	}

	return -1;	// very bad thing happened 	TODO
}

void _tty_clear_buffer(tty_t* tty){
	// clear the tty buffer
	tty->buf.end = (tty->buf.index + TTY_BUF_LENGTH - 1) % TTY_BUF_LENGTH;
}

void tty_send_input(uint8_t* data, uint32_t size){
	// input is always sent to the current foreground tty
	uint32_t i;
	uint32_t print_size = 0;
	tty_buf_t* op_buf = &(cur_tty->buf);
	uint32_t keyboard_pid_waiting = cur_tty->input_pid_waiting;
	// buffer handle
	for (i=0; i<size; ++i){
		// accept Ctrl C even if overflowed
		if (data[i] == 3){
			// send signal to end that process
			syscall_kill(cur_tty->fg_proc, SIGTERM, 0);
			cur_tty->fg_proc = (task_list + cur_tty->fg_proc)->parent;
			// clear buffer for the tty
			_tty_clear_buffer(cur_tty);
			continue;
		}
		// special case for backspace, put into temp_buf but delete in tty buffer
		if (data[i] == '\b'){
			// don't backspace over the start
			if (((op_buf->index + TTY_BUF_LENGTH - 1)%TTY_BUF_LENGTH) != op_buf->end){
				op_buf->index = (op_buf->index + TTY_BUF_LENGTH - 1)%TTY_BUF_LENGTH;
				temp_buf[i] = data[i];
				print_size ++;
			}
		}else if (data[i] == '\n' ){
			if (op_buf->index == op_buf->end){
				// reaching the end of the buffer, abort
				// note that the last char of the buffer has not yet been written
				break;
			}
			// handle the char normally, but notify waiting process if there is
			temp_buf[i] = data[i];
			print_size ++;
			op_buf->buf[op_buf->index] = data[i];
			op_buf->index = (op_buf->index + 1) % TTY_BUF_LENGTH;
			if (keyboard_pid_waiting){
				syscall_kill(keyboard_pid_waiting, SIGIO, 0);
				cur_tty->input_pid_waiting = 0;
				keyboard_pid_waiting = 0;
			}
		}else if (data[i] == 12){
			// for clear screen, go to stdout, but don't go to buffer
			temp_buf[i] = data[i];
			print_size ++;
		}else if (data[i] >= 32 && data[i] <= 126){ // filter out unprintable chars
			if (op_buf->index == op_buf->end){
				// reaching the end of the buffer, abort
				// note that the last char of the buffer has not yet been written
				break;
			}
			// printable characters go in
			temp_buf[i] = data[i];
			print_size ++;
			op_buf->buf[op_buf->index] = data[i];
			op_buf->index = (op_buf->index + 1) % TTY_BUF_LENGTH;
		}
	}
	// special case for the last enter
	if ((op_buf->index == op_buf->end) && (i<size) && (data[i] == '\n')){
		op_buf->buf[op_buf->index] = '\n';
		op_buf->flags |= TTY_BUF_ENTER;
		// we don't increment index this case to block further input, but we add i
		temp_buf[i] = '\n'; 	// temp buffer used for stdout
		print_size++;
		syscall_kill(keyboard_pid_waiting, SIGIO, 0);
		cur_tty->input_pid_waiting = 0;
		keyboard_pid_waiting = 0;
	}
	// if echo flag is on then call write
	if (!(cur_tty->flags & TTY_FG_ECHO)){
		tty_stdout(temp_buf, print_size, cur_tty->output_private_data);
		terminal_set_cursor(cur_tty->output_private_data);
	}
}

int tty_open(struct s_inode *inode, struct s_file *file){
	// set the private data to the process's tty
	task_t* proc = task_list + task_current_pid();
	file->private_data = proc->tty;
	return 0;
}

int tty_close(struct s_inode *inode, struct s_file *file){
	// do nothing
	return 0;
}

ssize_t tty_read(struct s_file *file, uint8_t *buf, size_t count, off_t *offset){
	// blocking read from tty buffer
	task_sigact_t sa;
	sigset_t ss;
	tty_buf_t* op_buf = &(tty_list[(task_list + task_current_pid())->tty].buf);
	uint32_t i;
	uint32_t copy_start = (op_buf->end + 1) % TTY_BUF_LENGTH;

	// check for enter in the buffer
	for (i = (op_buf->end + 1)%TTY_BUF_LENGTH; i != op_buf->index; i=(i+1)%TTY_BUF_LENGTH){
		if (op_buf->buf[i]=='\n'){
			op_buf->flags |= TTY_BUF_ENTER;
			break;
		}
	}

	if (!(op_buf->flags &  TTY_BUF_ENTER)){
		// No enter in buffer, set process to sleep until SIGIO
		tty_list[(task_list + task_current_pid())->tty].input_pid_waiting = task_current_pid();
		sa.handler = SIG_IGN;
		sigemptyset(&(sa.mask));
		sa.flags = SA_RESTART;
		syscall_sigaction(SIGIO, (int)&sa, 0);
		sigemptyset(&ss);
		syscall_sigsuspend((int) &ss, NULL, 0);
		return 0; // Should not hit
	}

	// copy until hit enter, with enter
	for (i=0; i<count; ++i){
		if (copy_start == op_buf->index) break;
		buf[i] = op_buf->buf[copy_start];
		copy_start = (copy_start + 1)%TTY_BUF_LENGTH;
		if (buf[i] == '\n') break;
	}
	op_buf->flags = op_buf->flags &  (~TTY_BUF_ENTER);
	// special case for overflowed buffer
	if (op_buf->index == op_buf->end){
		buf[i] = op_buf->buf[copy_start];
		// hold the end still, move the index to one after
		op_buf->index = (op_buf->end + 1) % TTY_BUF_LENGTH;
		return (i+1);
	}

	// guaranteed to have enter
	op_buf->end = (copy_start + TTY_BUF_LENGTH - 1) % TTY_BUF_LENGTH;

	return (i+1);
}

ssize_t tty_write(file_t *file, uint8_t *buf, size_t count, off_t *offset){
	ssize_t ret;
	// get tty number from file private data
	int tty_index = file->private_data;
	// write to tty that the process belongs to
	ret = tty_stdout(buf, count, tty_list[tty_index].output_private_data);
	if (cur_tty == (&tty_list[tty_index])){
		terminal_set_cursor(cur_tty->output_private_data);
	}
	return ret;
}
