CROSS_PREFIX=i386-saenaios-
CC=$(CROSS_PREFIX)gcc
AR=$(CROSS_PREFIX)ar
LD=$(CROSS_PREFIX)gcc
CFLAGS=-Wall -O2 -g -Werror -m32
LDFLAGS=-O2
ARFLAGS=rcs

all: libuserutils.a userutils

userutils: main.o libuserutils.a
	$(LD) $(LDFLAGS) $^ -o $@

libuserutils.a: sha256.o userutils.o
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f *.o *.a
