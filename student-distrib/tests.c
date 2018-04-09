#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "libc.h"

#include "boot/idt.h"
#include "keyboard.h"
#include "rtc.h"
#include "fsdriver/mp3fs_test.h"
#include "terminal_driver/terminal_out_driver.h"
#include "boot/page_table.h"
#include "boot/ece391_syscall.h"

#include "proc/task.h"
#include "boot/syscall.h"
#include "fs/vfs.h"
#include "fs/test.h"
#include "types.h"
#include "../libc/include/dirent.h"


static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 *
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 0x20; ++i){
		if ((idt[i].offset_15_00 == NULL) &&
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}
	if ((idt[0x80].offset_15_00 == NULL) &&
		(idt[0x80].offset_31_16 == NULL)){
		assertion_failure();
		result = FAIL;
	}
	return result;
}

// add more tests here

/*
 *	paging_test
 *		Inputs: None
 *		Outputs: Pass/Fail
 *		Side Effects: May crash the system
 *		Coverage: Page initialization
 */
int paging_test(){
	TEST_HEADER;

	int temp;
	// kernel space paging test
	temp = *(int*)(0x600000);
	// video memory paging test
	temp = *(int*)(0xB8500);
	// fs memory paging test
	//temp = *(int*)(0xA00000);
	printf("kernel memory & video memory paging test passed\n");

	// memory from 0 to the beginning of video memory crashing test
	//printf("crashing 0 - 0xB7FFF space!\n");
	//temp = *(int*)(0x10000);
	// memory after video memory to the beginning of kernel memory crashing test
	//printf("crashing 0xB9000 - 0x3FFFFF space!\n");
	//temp = *(int*)(0xBA000);
	// memory after kernel memory crashing test
	// printf("crashing 0xC00000 - 4GB space!\n");
	// temp = *(int*)(0xC00000);
	// assertion_failure();
	printf("now mapping two virtual address to the same physical address\n");

	page_dir_add_4MB_entry(0x08000000, 0xC00000, PAGE_DIR_ENT_PRESENT | PAGE_DIR_ENT_RDWR |
							PAGE_DIR_ENT_SUPERVISOR);
	page_dir_add_4MB_entry(0x08400000, 0xC00000, PAGE_DIR_ENT_PRESENT | PAGE_DIR_ENT_RDWR |
							PAGE_DIR_ENT_SUPERVISOR);
	*((int*)0x08000000) = 3;
	if (*((int*)0x08400000) != 3){
		printf("virtual memory paging failed\n");
		return FAIL;
	}
	*((int*)0x08400000) = 233;
	if (*((int*)0x08000000) != 233){
		printf("virtual memory paging failed\n");
		return FAIL;
	}

	// now change those entries

	page_dir_add_4MB_entry(0x08800000, 0x1000000, PAGE_DIR_ENT_PRESENT | PAGE_DIR_ENT_RDWR |
							PAGE_DIR_ENT_SUPERVISOR);
	*((int*)0x08800000) = 333;
	//printf("before %d\n", *((int*)0x08400000));

	page_dir_add_4MB_entry(0x08400000, 0x1000000, PAGE_DIR_ENT_PRESENT | PAGE_DIR_ENT_RDWR |
							PAGE_DIR_ENT_SUPERVISOR);
	//printf("after  %d\n", *((int*)0x08400000));

	// after changing the entry, tlb is not cleared, so
	// the original mapping remains
	if ((*((int*)0x08400000)) != 233){
		printf("tlb wrongly flushed\n");
		return FAIL;
	}
	// now flushed, the mapping should be changed
	page_flush_tlb();
	//printf("after flush %d\n", *((int*)0x08400000));
	if ((*((int*)0x08400000)) != 333){
		printf("tlb flush failed\n");
		return FAIL;
	}
	return PASS; // If execution hits this, BAD!
}

/**
 *	Test IDT by triggering a division error
 *
 *	@note This test will trigger a fault
 */
int idt_test_de() {
	TEST_HEADER;
	int n=5, m=0;
	printf("Triggering a div-by-error fault...\n");
	n /= m; // Should fault (DE)
	assertion_failure();
	return FAIL; // If execution hits this, BAD!
}

/**
 *	Test IDT by making a system call
 *
 *	@note This test should not trigger a fault
 */
int idt_test_usr() {
	TEST_HEADER;
	printf("Sending system call number 42...\n");
	asm volatile ("\
		movl $42, %eax\n\
		int $0x80\n\
	");
	return PASS;
}

/**
 *	Last key pressed
 *
 *	Helper for KB test
 */
volatile char kb_test_last_key;

/**
 *	Keyboard event handler for testing
 */
void kb_test_handler() {
	send_eoi(KBD_IRQ_NUM);
	unsigned char scancode;
	/* reads scancode */
	scancode = inb(DATA_REG);
	/* put the corresponding character on screen */
	if(!(scancode & RELEASE_OFFSET)) {
		kb_test_last_key = kbdreg[scancode];
		putc(kb_test_last_key);
	}
}

/**
 *	Test keyboard handler
 *
 *	@note This test depends on a working IDT. This test will also remove the
 *		  handler installed by `keyboard_init`.
 */
int kb_test() {
	TEST_HEADER;
	idt_removeEventListener(KBD_IRQ_NUM);
	kb_test_last_key = '\0';
	idt_addEventListener(KBD_IRQ_NUM, &kb_test_handler);
	printf("Type on the keyboard and text should show up on the screen\n");
	printf("Press backtick to end...\n");
	while(kb_test_last_key != '`');
	return PASS;
}

/**
 *	RTC event handler for testing
 */
void rtc_test_handler(){
	/* sends eoi */
	send_eoi(RTC_IRQ_NUM);
	/* calls tet function in lib.c */
	test_interrupts();
	/* reads from register C so that the interrupt will happen again */
	outb(REG_C, RTC_PORT);
	inb(CMOS_PORT);
}

/**
 *	Test RTC handler
 *
 *	@note This test depends on a working IDT and working keyboard. This test
 *		  will also remove the handler
 */
// int rtc_test() {
// 	TEST_HEADER;
// 	idt_removeEventListener(KBD_IRQ_NUM);
// 	idt_removeEventListener(RTC_IRQ_NUM);
// 	idt_addEventListener(KBD_IRQ_NUM, &kb_test_handler);
// 	printf("Screen will be garbled in this test.\n");
// 	printf("Press S to start, press E to end...\n");
// 	kb_test_last_key = '\0';
// 	while(kb_test_last_key != 's');
// 	idt_addEventListener(RTC_IRQ_NUM, &rtc_test_handler);
// 	rtc_open();
// 	rtc_setrate(0x06); // Slower!
// 	kb_test_last_key = '\0';
// 	while(kb_test_last_key != 'e');
// 	idt_removeEventListener(RTC_IRQ_NUM);
// 	clear();
// 	printf("RTC Test ended.\n");
// 	return PASS;
// }

/* Checkpoint 2 tests */

// Symbol defined in libc's do_syscall.S
extern int do_syscall(int num, int b, int c, int d);

/**
 *	Syscall 15 handler for testing
 *
 *	@param a, b, c: data. Should be sent 42, -391 and 0xecebcafe
 *	@return 0xac if data matched, 0xbad if it does not
 */
int test_syscall_handler(int a, int b, int c) {
	if (a == 42 && b == -391 && c == 0xecebcafe) {
		return 0xac;
	}
	printf("Syscall passed in bad values\n");
	return 0xbad;
}

/**
 *	Test system call dispatcher
 */
int test_syscall_dispatcher() {
	TEST_HEADER;
	// Using syscall number 15, the unused portion of the ece391 specification
	if (syscall_register(15, &test_syscall_handler) != 0) {
		printf("Failed to register handler\n");
		assertion_failure();
	}
	// Send dwords 42, -391 and 0xecebcafe, should return 0xac
	if (do_syscall(15, 42, -391, 0xecebcafe) != 0xac) {
		printf("Syscall 15 returned bad value\n");
		assertion_failure();
	}
	return PASS;
}

int test_stdio_with_fd(){
	int fd, pre_fd;
	char temp_buf[20];
	fd = open("/dev/stdout",O_RDONLY, 0);
	pre_fd = fd;
	struct dirent* temp_dirent;
	DIR* temp_dir;
	if (fd < 0){
		return FAIL;
	}
	char clear_sequence[4];
	clear_sequence[0] = 12;
	write(fd, clear_sequence, 1);

	write(fd, "You should see a cleared screen with this line!",47);

	printf("\nThis is a message sent by printf through stdout!\n");

	printf("Reading stdout...\nNow this should return 0: %d \n", read(fd,temp_buf,3));
	if (read(fd,temp_buf,3) != 0)
		assertion_failure();
	close(fd);

	printf("Writing to a closed stdout...\nShould be a error code: %d \n",write (fd, "see", 3));

	if (write (fd, "see", 3) != -EBADF)
		assertion_failure();

	//do it again!
	fd = open("/dev/stdout",O_RDONLY, 0);

	if (pre_fd!=fd) {
		printf("stdio: FD leak\n");
		//should give same fd for now
		return FAIL;
	}
	// now open dev file to go through dev directory
	if (!(temp_dir = opendir("/dev"))){
		printf("open dev directory failed\n");
		return FAIL;
	}
	if (!(temp_dirent = readdir(temp_dir))){
		printf("read dev directory failed\n");
		return FAIL;
	}
	// should be stdout
	if (strncmp(temp_dirent->filename, "stdout", 32)){
		printf("read stdout dentry failed\n");
		return FAIL;
	}


	return PASS;
}

/*
int stdin_test(){

	TEST_HEADER;
	printf("Testing stdin read. Press '`' to end test.\n");
	int fd, nbytes;
	uint8_t buffer[KEY_BUF_SIZE];
	keyboard_driver_register();
	fd = open("/dev/stdin",O_RDONLY, 0);
	if(fd < 0){
		printf("Unable to open device.");
	}
	while(buffer){
		memset(buffer, 0, KEY_BUF_SIZE);
		while(read(fd, buffer, nbytes) < 0);
		if(buffer[0]=='`') break;
		terminal_print(buffer);
	}
	clear();
	close(fd);
	return PASS;
}
*/

int rtc_test_2() {
	TEST_HEADER;
	int i, j;
	int fd;
	int freq = 2;
	char tick[] = "0";
	idt_removeEventListener(KBD_IRQ_NUM);
	idt_removeEventListener(RTC_IRQ_NUM);
	idt_addEventListener(RTC_IRQ_NUM, &rtc_handler);
	idt_addEventListener(KBD_IRQ_NUM, &kb_test_handler);
	rtc_out_driver_register();

	terminal_print("RTC Test. Will print '1's at increasing RTC rates.\n"
				   "Press Enter to begin...");
	kb_test_last_key = '\0';
	while(kb_test_last_key != '\n');

	fd = open("/dev/rtc", O_RDWR, 0);
	if (fd < 0) {
		printf("Failed to open RTC %d\n", fd);
		return FAIL;
	}

	for (i = 0; i < 10; i++) {
		write(fd, &freq, sizeof(freq));
		clear();
		for (j = 0; j < 79; j++) {
			read(fd, &freq, 0); // Read should not modify anything
			printf("1");
		}
		tick[0]++;
		freq *= 2;
	}
	clear();

	close(fd);

	// idt_removeEventListener(RTC_IRQ_NUM);
	return PASS;
}

/**
 *	Test keyboard read handler
 *
 *	Enable keyboard testing mode, each keypress of enter key
 *	will evoke keyboard_read to read from the keyboard buffer
 *	and output the values the terminal
 */

int test_keyboard_read() {
	TEST_HEADER;

	char buf[128 + 1];
	int fd_in, fd_out, count_in, count_out, ret;

	ret = keyboard_driver_register();
	if (ret != 0 && ret != -EEXIST) {
		printf("Failed to register stdin driver (err=%d)\n", ret);
		return FAIL;
	}
	ret = terminal_out_driver_register();
	if (ret != 0 && ret != -EEXIST) {
		printf("Failed to register stdout driver (err=%d)\n", ret);
		return FAIL;
	}

	ret = PASS;
	fd_in = open("/dev/stdin", O_RDONLY, 0);
	fd_out = open("/dev/stdout", O_WRONLY, 0);
	if (fd_out < 0) {
		printf("kb: cannot open stdout (err=%d)\n", fd_out);
		ret = FAIL;
		goto cleanup;
	}
	if (fd_in < 0) {
		printf("kb: cannot open stdin (err=%d)\n", fd_in);
		ret = FAIL;
		goto cleanup;
	}
	printf("Dumping stdin to stdout (equiv. cat /dev/stdin)...\n"
		   "Enter 'exit' to finish\n");
	memset(buf, '\0', sizeof(buf));
	while (strncmp(buf, "exit", 4) != 0) {
		count_in = read(fd_in, buf, 128);
		if (count_in < 0) {
			printf("kb: read failed (err=%d)\n", count_in);
			ret = FAIL;
			break;
		}
		count_out = write(fd_out, buf, count_in);
		if (count_out < 0) {
			printf("kb: write failed (err=%d)\n", count_out);
			ret = FAIL;
			break;
		} else if (count_out != count_in) {
			printf("kb:write could not flush all bytes.\n"
				   "read %d bytes, written %d bytes\n", count_in, count_out);
			ret = FAIL;
			break;
		}
	}

cleanup:
	if (fd_in >= 0) {
		fd_in = close(fd_in);
		if (fd_in != 0) {
			printf("kb: fail to close stdin (err=%d)\n", fd_in);
			ret = FAIL;
		}
	}
	if (fd_out >= 0) {
		fd_out = close(fd_out);
		if (fd_out != 0) {
			printf("kb: fail to close stdout (err=%d)\n", fd_out);
			ret = FAIL;
		}
	}

	return ret;
}

/* Checkpoint 3 tests */

/**
 *	Testing pcb functions
 *
 *	@return 0 on fail, 1 on pass
 */
 /*
int test_pcb(){
	TEST_HEADER;
	proc_init();
	pcb_t* test_pcb;
	printf("Checking address and pid of current pcb.\n");
	test_pcb = get_curr_pcb();
	printf("Current pcb address is %x, pid: %d.\n", (int)test_pcb, test_pcb->curr_pid);
	printf("Checking some invalid inputs.\n");
	if(get_pcb_addr(-1)==-1){
		printf("Invalid process number!\n");
	}
	return PASS;
}*/

/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests() {
	irq_listener kbd_orig, rtc_orig;

	// Save and clear KB and RTC handlers
	kbd_orig = idt_getEventListener(KBD_IRQ_NUM);
	rtc_orig = idt_getEventListener(RTC_IRQ_NUM);
	idt_removeEventListener(KBD_IRQ_NUM);
	idt_removeEventListener(RTC_IRQ_NUM);

	// Begin testing

	clear();
	printf("Running tests...\n");

	//TEST_OUTPUT("syscall_devfs_stdout_test", test_stdio_with_fd());

	TEST_OUTPUT("paging test",paging_test());

	// IDT tests
	//TEST_OUTPUT("idt_test", idt_test());
	// The following test should trigger a fault
	// TEST_OUTPUT("idt_test_de", idt_test_de());
	// The following test should not trigger any fault
	//TEST_OUTPUT("idt_test_usr", idt_test_usr());

	// Paging test
	// The following test should trigger a fault
	// TEST_OUTPUT("paging_test", paging_test());

	// KB test
	//TEST_OUTPUT("kb_test", kb_test());

	// RTC test
	//TEST_OUTPUT("rtc_test", rtc_test());

	// Restore IRQ Handlers
	idt_removeEventListener(KBD_IRQ_NUM);
	idt_removeEventListener(RTC_IRQ_NUM);
	idt_addEventListener(KBD_IRQ_NUM, kbd_orig);
	idt_addEventListener(RTC_IRQ_NUM, rtc_orig);

	TEST_OUTPUT("Syscall dispatcher test", test_syscall_dispatcher());

	// File and directory test

	TEST_OUTPUT("mp3fs driver test", launch_mp3fs_driver_test());

	fs_test();

	// TEST_OUTPUT("test_keyboard_read", test_keyboard_read());

	// TEST_OUTPUT("rtc_test_2", rtc_test_2());
}
