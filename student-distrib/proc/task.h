/**
 *	@file proc/task.h
 *
 *	Data structures tracking tasks/processes.
 */
#ifndef PROC_TASK_H
#define PROC_TASK_H

#include "../types.h"
#include "../fs/vfs.h"
#include "../libc.h"
#include "../boot/page_table.h"
#include "../boot/syscall.h"
#include "../boot/idt_int.h"

#include "../../libc/include/signal.h"

#define TASK_ST_NA			0	///< Process PID is not in use
#define TASK_ST_RUNNING		1	///< Process is actively running on processor
#define TASK_ST_SLEEP		2	///< Process is sleeping
#define TASK_ST_ZOMBIE		3	///< Process is awaiting parent `wait()`
#define TASK_ST_DEAD		4	///< Process is dead

#define TASK_MAX_PROC		64	///< Maximum of concurrently-scheduled tasks
#define TASK_MAX_OPEN_FILES	16		///< Per-process limit of concurrent open files
#define TASK_MAX_PAGE_MAPS	16		///< Maximum pages a process may request

#define TASK_PTENT_CPONWR	0x1		///< Current page is copy-on-write

#define TASK_MAX_HEAP		0x2800000	///< max heap size a process is allowed to allocate

/**
 *	A mapped memory page in a process's mapped page table
 */
typedef struct s_task_ptentry {
	uint32_t vaddr; ///< Virtual address visible to the process
	uint32_t paddr; ///< Physical address to write into the page table
	uint16_t pt_flags; ///< Flags passed to `page_dir_add_*_entry`
	uint16_t priv_flags; ///< Private flags
} task_ptentry_t;

/**
 * 	A structure to store the data segment allocation info,
 * 	used for brk, sbrk
 */
typedef struct s_heap_desc {
	uint32_t 	start;				///< virtual address of last allocated 4MB
	uint32_t 	prog_break;			///< pointer to the end of heap + 1
} heap_desc_t;

/**
 *	Structure for a process in the PID table
 */
typedef struct s_task {
	uint8_t status;		///< Current status of this task
	uint8_t tty; 		///< Attached tty number
	pid_t pid;			///< current process id
	pid_t parent;		///< parent process id

	regs_t regs;		///< Registers stored for current process

	file_t *files[TASK_MAX_OPEN_FILES]; ///< File descriptor pool

	task_ptentry_t pages[TASK_MAX_PAGE_MAPS]; ///< Mapped pages
	uint32_t vidmap;		///< for the damn video map
	uint32_t vidpage_index;	///< for the damn video map

	uint32_t 	ks_esp;	///< Kernel Stack pointer
	struct s_heap_desc heap; 	///< heap descriptor

	struct sigaction sigacts[SIG_MAX]; ///< Signal handlers
	sigset_t signals;	///< Pending signals
	sigset_t signal_mask; ///< Deferred signals
	uint32_t exit_status; ///< Status to report on `wait`

	uid_t uid; ///< User ID of the process
	gid_t gid; ///< Group ID of the process
	
	char *wd; ///< Working directory
} task_t;

/**
 *	List of processes
 */
extern task_t task_list[TASK_MAX_PROC];

/**
 *	Get the PID of the currently executing process
 *
 *	@note: this process currently always return 0 for the kernel code
 *
 *	@return the PID.
 */
pid_t task_current_pid();

/**
 *	Initialize pid 0 for kernel code
 */
void task_create_kernel_pid();

/**
 *	Start process system
 */
void task_start_kernel_pid();

/**
 *	Find an available pid
 *
 *	@return the new pid, or -EAGAIN if all pids are in use (probably very bad)
 */
int16_t task_alloc_pid();

/**
 *	Get current process PID
 *
 *	@return the pid
 */
int syscall_getpid(int, int, int);

/**
 *	Fork current process
 *
 *	@return The new pid on success, or the negative of an errno on failure
 */
int syscall_fork(int, int, int);

/**
 *	Execute a new file with the current process
 *
 *	@param pathp: pointer to `char *`, the path to the executable file
 *	@param argvp: pointer to `char *[]`, an array of arguments
 *	@param envpp: pointer to `char *[]`, an array of environment variables
 *	@return The caller will no longer exist to receive the return value if the
 *			call succeeded. The negative of an errno is returned on failure.
 *	@note argv and envp are NULL-terminated arrays
 */
int syscall_execve(int pathp, int argvp, int envpp);

/**
 *	Exit current process
 *
 *	@param status: the exit code
 *	@return This call will not return
 */
int syscall__exit(int status, int, int);

/**
 *	Wait for child process to change status
 *
 *	@param pid: the pid to wait for, or -1 to wait for all child processes
 *	@param statusp: pointer to an `int` buffer. The child's status will be
 *					populated into this buffer
 *	@param options: bit map of option flags
 */
int syscall_waitpid(int pid, int statusp, int options);

/**
 *	syscall to extend end of process' data segment to the
 *	specified program break
 *
 *  program break is the first location after the end of the uninitialized data segment
 *
 *	@param paddr: address of the expected program break;
 *	@param b: placeholder
 *	@param c: placeholder
 *	@return 0 on success, -1 on error, specific error number stored in errno
 *	@note test passed on non-4MB-aligned start, now 64MB at max, limited by pages number in task_t
 */
int syscall_brk(int paddr, int b, int c);

/**
 *	syscall to extend program break by a given amount
 *
 *	@param increment: amount of change to the program break, could be negative value
 *	@param b: placeholder
 *	@param c: placeholder
 *	@return the address of previous program break, (void*)-1 on error,
 *		specific error number stored in errno
 *	@note test passed on non-4MB-aligned start, now 64MB at max, limited by pages number in task_t
 */
int syscall_sbrk(int increment, int b, int c);

/**
 *	ECE391 (`system` style) execute wrapper
 *
 *	@param cmdlinep: pointer to `char *`, the entire command line
 *	@return 0 on success, or the negative of an error on failure
 *	@note This call emulates the ece391_execute by appending a new frame
 *		  executing `sigsuspend` onto the current user stack, so that the user
 *		  code will not resume execution until the child process has terminated.
 */
int syscall_ece391_execute(int cmdlinep, int, int);

/**
 *	ECE391 halt wrapper
 *
 *	@return This call will not return
 */
int syscall_ece391_halt(int, int, int);

/**
 *	Getargs syscall handler
 *
 *	@param buf: buffer to copy arguments
 *	@param nbytes: number of bytes to copy
 *	@return -1 on error, 0 on success
 */
int syscall_ece391_getargs(int buf, int nbytes, int);

/**
 *	System call handler for `getcwd`: Get current working directory
 *
 *	@param bufp: the buffer to read cwd into
 *	@param size: the max size of the buffer
 *	@return 0 on success, or the negative of an errno on failure
 */
int syscall_getcwd(int bufp, int size, int);

/**
 *	System call handler for `chdir`: Change current working directory
 *
 *	@param pathp: address of path to the new working directory
 *	@return 0 on success, or the negative of an errno on failure.
 */
int syscall_chdir(int pathp, int, int);

/**
 *	Release a process from memory
 *
 *	This call will free up all system resources (memory pages, file descriptors)
 *	previously allocated for the process, and release the memory of the `task_t`
 *	itself.
 *
 *	This function should NOT be used unless an absolutely unrecoverable error
 *	occurs to the process that a graceful kill is impossible. To gracefully kill
 *	a process, use `syscall_kill` to terminate it using a `SIGKILL`.
 *
 *	@param proc: the `task_t` structure to be released
 */
void task_release(task_t *proc);

/**
 *	Push buffer onto the user stack. Update user esp.
 *
 *	@param esp: pointer to user esp
 *	@param buf: data buffer to be pushed
 *	@param size: size in bytes of the data buffer
 *	@return 0 on success. -EFAULT if user stack overflow
 *	@note If an overflow would happen, the user stack will enter an undefined
 *		  making executing any further user code impossible, including the
 *		  SIGSEGV handler (a SIGSEGV endless recursion would occur). The caller
 *		  should directly kill the process by resetting its SIGSEGV handler to
 *		  SIG_DFL kernel default handler and send it a SIGSEGV to gracefully
 *		  report such error to its possibly waiting parent.
 */
int task_user_pushs(uint32_t *esp, uint8_t *buf, size_t size);

/**
 *	Push dword onto the user stack. Update user esp.
 *
 *	@param esp: pointer to user esp
 *	@param val: 32-bit value to be pushed
 *	@return 0 on success. -EFAULT if user stack overflow
 *	@see task_user_pushs
 */
int task_user_pushl(uint32_t *esp, uint32_t val);

/**
 *	Test process memory access permission for address
 *
 *	@param addr: the address to be tested
 *	@return 0 if access granted, or the negative of an errno if denied
 */
int task_access_memory(uint32_t addr);

/**
 *	Perform copy-on-write if applicable on page fault
 *
 *	@param addr: the faulting address
 *	@return 0 if the page fault is resolved and the caller should resume the
 *			  process, or non-zero if the page fault is not on a copy-on-write
 *			  page and the process should indeed be sent a SIGSEGV
 */
int task_pf_copy_on_write(uint32_t addr);

/**
 *	Initialize the initd process
 *
 *	@return nothing
 */
int task_make_initd(int, int, int);

#endif
