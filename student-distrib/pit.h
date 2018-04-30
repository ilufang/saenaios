/**
 *	@file pit.h
 *
 *	Programmable Interrupt Timer chip driver
 */
#ifndef PIT_H
#define PIT_H

/**
 *	Set PIT to generate periodic interrupt
 *
 *	@param rate: number of interrupts to generate per second
 */
void pit_setrate(int rate);

/**
 *	Initialize PIT
 */
void pit_init();

#endif
