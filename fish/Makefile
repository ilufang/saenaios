all: fish

# Note that you must be superuser to run the emulated version of the
# program.

fish_emulated: fish.o blink.o ece391emulate.o ece391support.o
	gcc -nostdlib -lc -g -o fish_emulated fish.o blink.o ece391emulate.o ece391support.o

fish: fish.exe
	mv fish.exe ../fsdir/fish

fish.exe: fish.o blink.o ece391support.o ece391syscall.o
	i386-saenaios-gcc -nostdlib -g -o fish.exe fish.o blink.o ece391syscall.o ece391support.o

%.o: %.S
	i386-saenaios-gcc -nostdlib -c -Wall -g -D_USERLAND -D_ASM -o $@ $<

%.o: %.c
	i386-saenaios-gcc -nostdlib -Wall -c -g -o $@ $<

clean::
	rm -f *.o *~
clear: clean
	rm -f fish fish.exe fish_emulated
