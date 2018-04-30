/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = 0xFF; /* IRQs 0-7  */
uint8_t slave_mask = 0xFF;  /* IRQs 8-15 */

/* PIC date ports are address ports + 1 */
#define MASTER_8259_DATA MASTER_8259_PORT+1
#define SLAVE_8259_DATA SLAVE_8259_PORT+1

#define BIT_MASK        0x1
#define BYTE_MASK       0xFF
#define SLAVE_OFFSET    7
#define NUM_LINES       8

void i8259_init(void) {

	/* clear all masks on PIC */
	outb(BYTE_MASK, MASTER_8259_DATA);
	outb(BYTE_MASK, SLAVE_8259_DATA);

	/* write the 4 control words */
	outb(ICW1, MASTER_8259_PORT);
	outb(ICW1, SLAVE_8259_PORT);

	outb(ICW2_MASTER, MASTER_8259_DATA);
	outb(ICW2_SLAVE, SLAVE_8259_DATA);


	outb(ICW3_MASTER, MASTER_8259_DATA);
	outb(ICW3_SLAVE, SLAVE_8259_DATA);


	outb(ICW4, MASTER_8259_DATA);
	outb(ICW4, SLAVE_8259_DATA);

	/* set mask */
	outb(master_mask, MASTER_8259_DATA);
	outb(slave_mask, SLAVE_8259_DATA);

}

void enable_irq(uint32_t irq_num) {

	uint32_t mask = BIT_MASK;
	// uint32_t ms_mask = BIT_MASK;

	/* the irq line is on master PIC */
	if(irq_num<=SLAVE_OFFSET){
		/* clear the corresponding bit in the mask */
		mask = mask << irq_num;
		mask = ~mask;
		master_mask &= mask;
		/* write the new mask */
		outb(master_mask, MASTER_8259_DATA);
	}
	/* the irq line is on slave PIC */
	else{
		/* compute and clear the corresponding bit for slave PIC */
		mask = mask << (irq_num - NUM_LINES);
		mask = ~mask;
		slave_mask &= mask;
		/* unmask the irq line on master that cascades to slave */
		enable_irq(SLAVE_PORT);
		/* write the new mask */
		outb(slave_mask, SLAVE_8259_DATA);
	}

}

void disable_irq(uint32_t irq_num) {

	uint32_t mask = BIT_MASK;

	/* the irq line is on master */
	if(irq_num<=SLAVE_OFFSET){
		/* set the corresponding bit */
		mask = mask << irq_num;
		master_mask |= mask;
		/* write the new mask */
		outb(master_mask, MASTER_8259_DATA);
	}
	/* the irq line is on slave */
	else{
		/* compute and set the corresponding bit */
		mask = mask << (irq_num - NUM_LINES);
		slave_mask |= mask;
		//disable_irq(SLAVE_PORT);
		/* write the new mask */
		outb(slave_mask, SLAVE_8259_DATA);
	}

}

void send_eoi(uint32_t irq_num) {

	if(irq_num<=7){
		outb(EOI|irq_num, MASTER_8259_PORT);
	}
	else{
		outb(EOI|(irq_num-8), SLAVE_8259_PORT);
		outb(EOI|SLAVE_PORT, MASTER_8259_PORT);
	}

}
