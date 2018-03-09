/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = 0xFF; /* IRQs 0-7  */
uint8_t slave_mask = 0xFF;  /* IRQs 8-15 */

#define MASTER_8259_DATA MASTER_8259_PORT+1
#define SLAVE_8259_DATA SLAVE_8259_PORT+1

/* Initialize the 8259 PIC */
void i8259_init(void) {

    outb(0xFF, MASTER_8259_DATA);
    outb(0xFF, SLAVE_8259_DATA);
    
    outb(ICW1, MASTER_8259_PORT);
    outb(ICW1, SLAVE_8259_PORT);
    
    outb(ICW2_MASTER, MASTER_8259_DATA);
    outb(ICW2_SLAVE, SLAVE_8259_DATA);
    
    
    outb(ICW3_MASTER, MASTER_8259_DATA);
    outb(ICW3_SLAVE, SLAVE_8259_DATA);
    
    
    outb(ICW4, MASTER_8259_DATA);
    outb(ICW4, SLAVE_8259_DATA);
    
    outb(master_mask, MASTER_8259_DATA);
    outb(slave_mask, SLAVE_8259_DATA);

}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
    
    uint32_t mask = 0x1;
    uint32_t ms_mask = 0x1;
    
    if(0<=irq_num && irq_num<=7){
        mask = mask << irq_num;
        mask = ~mask;
        master_mask &= mask;
        outb(master_mask, MASTER_8259_DATA);
    }
    else{
        mask = mask << (irq_num - 8);
        mask = ~mask;
        slave_mask &= mask;
        enable_irq(SLAVE_PORT);
        outb(slave_mask, SLAVE_8259_DATA);
    }
    
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
    
    uint32_t mask = 0x1;
    
    if(0<=irq_num && irq_num<=7){
        mask = mask << irq_num;
        master_mask |= mask;
        outb(master_mask, MASTER_8259_DATA);
    }
    else{
        mask = mask << (irq_num - 8);
        slave_mask |= mask;
        disable_irq(SLAVE_PORT);
        outb(slave_mask, SLAVE_8259_DATA);
    }
    
}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
    
    if(0<=irq_num && irq_num<=7){
        outb(EOI|irq_num, MASTER_8259_PORT);
    }
    else{
        outb(EOI|(irq_num-8), SLAVE_8259_PORT);
        outb(EOI|SLAVE_PORT, MASTER_8259_PORT);
    }
    
}
