#include "tests.h"
#include "x86_desc.h"
#include "lib.h"

#include "boot/idt.h"
#include "keyboard.h"
#include "rtc.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

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
	printf("kernel memory & video memory paging test passed\n");

	// memory from 0 to the beginning of video memory crashing test
	//printf("crashing 0 - 0xB7FFF space!\n");
	//temp = *(int*)(0x10000);
	// memory after video memory to the beginning of kernel memory crashing test
	//printf("crashing 0xB9000 - 0x3FFFFF space!\n");
	//temp = *(int*)(0xBA000);
	// memory after kernel memory crashing test
	printf("crashing 0x800000 - 4GB space!\n");
	temp = *(int*)(0x800000);
	assertion_failure();
	return FAIL; // If execution hits this, BAD!
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
int rtc_test() {
	TEST_HEADER;
	idt_removeEventListener(KBD_IRQ_NUM);
	idt_removeEventListener(RTC_IRQ_NUM);
	idt_addEventListener(KBD_IRQ_NUM, &kb_test_handler);
	printf("Screen will be garbled in this test.\n");
	printf("Press S to start, press E to end...\n");
	kb_test_last_key = '\0';
	while(kb_test_last_key != 's');
	idt_addEventListener(RTC_IRQ_NUM, &rtc_test_handler);
	rtc_setrate(14); // Slower!
	kb_test_last_key = '\0';
	while(kb_test_last_key != 'e');
	idt_removeEventListener(RTC_IRQ_NUM);
	clear();
	printf("RTC Test ended.\n");
	return PASS;
}

/* Checkpoint 2 tests */
/* Checkpoint 3 tests */
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

	// IDT tests
	TEST_OUTPUT("idt_test", idt_test());
	// The following test should trigger a fault
	TEST_OUTPUT("idt_test_de", idt_test_de());
	// The following test should not trigger any fault
	TEST_OUTPUT("idt_test_usr", idt_test_usr());

	// Paging test
	// The following test should trigger a fault
	TEST_OUTPUT("paging_test", paging_test());

	// KB test
	TEST_OUTPUT("kb_test", kb_test());

	// RTC test
	TEST_OUTPUT("rtc_test", rtc_test());

	// Restore IRQ Handlers
	idt_removeEventListener(KBD_IRQ_NUM);
	idt_removeEventListener(RTC_IRQ_NUM);
	idt_addEventListener(KBD_IRQ_NUM, kbd_orig);
	idt_addEventListener(RTC_IRQ_NUM, rtc_orig);
}

void temp_terminal_test(){
	terminal_out_open();
	int i;
	char temp_buf[30]="^[*f**king****test^[*";
	//int i;
	temp_buf[2] = 12;
	temp_buf[20] = 13;
	terminal_out_write((uint8_t*)temp_buf,21);
	temp_buf[2] = 8;
	terminal_out_write((uint8_t*)temp_buf,21);
	temp_buf[20] = '*';
/*	for (i=0;i<30;++i){
		temp_buf[18] = i%10 + '0';
		terminal_out_write((uint8_t*)temp_buf,21);
	}*/
	while(1);
}
