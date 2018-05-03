# Makefile for OS project
# To build, first `make dep`, them `make`. Everything should be automatic.
# Will compile all *.c and *.S files in the current directory.


# Flags to use when compiling, preprocessing, assembling, and linking
CFLAGS=-std=gnu89 -ffreestanding -O2 -Wall -Wextra -g -Wno-unused-parameter
ASFLAGS=
LDFLAGS=-ffreestanding -O2 -nostdlib
CC=i386-elf-gcc
LD=i386-elf-gcc

#If you have any .h files in another directory, add -I<dir> to this line
CPPFLAGS+=-nostdinc -g -I../libc/include

# This generates the list of source files
SRC=$(wildcard *.S) $(wildcard *.c) $(wildcard */*.S) $(wildcard */*.c)

# This generates the list of .o files. The order matters, boot.o must be first
OBJS=boot.o
OBJS+=$(filter-out boot.o,$(patsubst %.S,%.o,$(filter %.S,$(SRC))))
OBJS+=$(patsubst %.c,%.o,$(filter %.c,$(SRC)))
OBJS+=../fuse-lwext4/lwext4/src/libext4.a

bootimg: Makefile $(OBJS)
	rm -f bootimg
	$(LD) -T linker.ld $(LDFLAGS) $(OBJS) -o bootimg -lgcc

dep: Makefile.dep

Makefile.dep: $(SRC)
	$(CC) -MM $(CPPFLAGS) $(SRC) > $@

.PHONY: clean
clean:
	rm -f *.o */*.o Makefile.dep

ifneq ($(MAKECMDGOALS),dep)
ifneq ($(MAKECMDGOALS),clean)
include Makefile.dep
endif
endif
