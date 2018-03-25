#ifndef TESTS_H
#define TESTS_H

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

#include "terminal_driver/terminal_out_driver.h"
// test launcher
void launch_tests();

#endif /* TESTS_H */
