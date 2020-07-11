
CFLAGS = -O3 -Wall -Werror -march=68000 -mcpu=68000
ASFLAGS = -march=68000 -mcpu=68000

CC = m68k-amigaos-gcc
AS = m68k-amigaos-as
LD = m68k-amigaos-ld
AR = m68k-amigaos-ar
AQB = ../../compiler/aqb

.PHONY: clean all diss

all: _aqb.a alibtest startup.o _aqb.sym

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

_aqb.sym : _aqb.bas
	$(AQB) -n -S $<
	pushd .. && rm -f _aqb.sym && ln -s _aqb/_aqb.sym && popd

clibtest.s:	clibtest.c
	$(CC) -S $(CFLAGS) $(CPPFLAGS) $< -o $@

amath.s:	amath.c
	$(CC) -S $(CFLAGS) $(CPPFLAGS) $< -o $@

%.o : %.s
	$(AS) $(ASFLAGS) $< -o $@

_aqb.a: cstartup.o aio.o astr.o amath.o minos.o autil.o
	$(AR) -crs _aqb.a $^

alibtest: startup.o alibtest.o clibtest.o _aqb.a
	$(LD) $^ -o $@
	cp $@ /home/guenter/media/emu/amiga/a500/hdd/system/x/
	# $(CC) $(CFLAGS) $(CPPFLAGS) -mcrt=nix13 -O alibtest.c -o alibtest

diss: alibtest
	m68k-amigaos-objdump -S -d -x alibtest

clean:
	rm -f *.o alibtest _aqb.a *.sym

