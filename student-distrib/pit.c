#include "pit.h"

#include "boot/idt.h"
#include "lib.h"
#include "i8259.h"
#include "proc/scheduler.h"

#define PIT_IRQNUM		0	///< IRQ number the PIT is connected to

#define PIT_DATAPORT0	0x40	///< Data port for counter 0
#define PIT_DATAPORT1	0x41	///< Data port for counter 1
#define PIT_DATAPORT2	0x42	///< Data port for counter 2
#define PIT_CMDPORT		0x43	///< Command port

int counter;

void pit_handler() {
	send_eoi(PIT_IRQNUM);
	if (scheduler_on_flag) {
		scheduler_event();
	}
}

void pit_init() {
	pit_setrate(512);
	idt_addEventListener(PIT_IRQNUM, pit_handler);
}

void pit_setrate(int rate)
{
	// Calculate divisor relative to default rate
	// Default rate is 1.193182 MHz
	rate = 1193182 / rate;
	
	// Set registers
	outb(0x36, PIT_CMDPORT); // Use counter 0 as rate generator
	outb(rate & 0xff, PIT_DATAPORT0); // LOBYTE
	outb(rate >> 8, PIT_DATAPORT0); // HIBYTE
}
