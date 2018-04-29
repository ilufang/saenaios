CC=i386-elf-gcc
LD=i386-elf-gcc
SYSROOT=../sysroot/i386-saenaios
CFLAGS = -Wall -nostdlib -ffreestanding -g -I$(SYSROOT)/include
LDFLAGS = -nostdlib -g -ffreestanding -L$(SYSROOT)/lib
LDLIBS = -lc -lm -lg -lgcc -lnosys

%.exe: %.o
	$(LD) $(LDFLAGS) -o $@ $^ $(SYSROOT)/lib/crt0.o $(LDLIBS)
	printf '\x03\x91' | dd of=$@ bs=1 seek=7 count=2 conv=notrunc

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $^

clean:
	rm -f *.o *.exe

.PHONY: clean

