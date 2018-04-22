CFLAGS += -Wall -nostdlib -ffreestanding -g
LDFLAGS += -nostdlib -ffreestanding
ARFLAGS = rcs
CC = gcc
LD = gcc
AR = ar

lib391c.a: errno.o do_syscall.o syscalls.o
	$(AR) $(ARFLAGS) "/tmp/$@" $^
	mv "/tmp/$@" $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -c -Wall -o $@ $<

clean::
	rm -f *~ *.o libc.a
