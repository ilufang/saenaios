/* types.h - Defines to use the familiar explicitly-sized types in this
 * OS (uint32_t, int8_t, etc.).  This is necessary because we don't want
 * to include <stdint.h> when building this OS
 * vim:ts=4 noexpandtab
 */

#ifndef _TYPES_H
#define _TYPES_H

#define NULL 0

#ifndef ASM

#include "../libc/include/stdint.h"
#include "../libc/include/stddef.h"
#include "../libc/include/sys/types.h"

#endif /* ASM */

#endif /* _TYPES_H */
