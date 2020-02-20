
CFLAGS = -O -Wall -march=68000 -mcpu=68000
ASFLAGS = -march=68000 -mcpu=68000

CC = m68k-amigaos-gcc
AS = m68k-amigaos-as
LD = m68k-amigaos-ld
AR = m68k-amigaos-ar

.PHONY: clean all diss

all: _alib.a alibtest startup.o

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.o : %.s
	$(AS) $(ASFLAGS) $< -o $@

_alib.a: io.o lowlevel_io.o astring.o math.o
	$(AR) -crs _alib.a $^

alibtest: startup.o alibtest.o _alib.a 
	$(LD) $^ -o $@
	cp $@ /home/guenter/media/emu/amiga/a500/hdd/system/x/
	# $(CC) $(CFLAGS) $(CPPFLAGS) -mcrt=nix13 -O alibtest.c -o alibtest

diss: alibtest
	m68k-amigaos-objdump -S -d -x alibtest

clean:
	rm -f *.o alibtest _alib.a

