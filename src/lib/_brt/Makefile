
CFLAGS = -O3 -Wall -Werror -march=68000 -mcpu=68000
ASFLAGS = -march=68000 -mcpu=68000

CC = m68k-amigaos-gcc
AS = m68k-amigaos-as
LD = m68k-amigaos-ld
AR = m68k-amigaos-ar
AQB = ../../compiler/aqb

.PHONY: clean all diss

all: _brt.a startup.o _brt.sym

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

_brt.sym : _brt.bas
	$(AQB) -d none -S $<
	pushd .. && rm -f _brt.sym && ln -s _brt/_brt.sym && popd

%.o : %.s
	$(AS) $(ASFLAGS) $< -o $@

_brt.a: cstartup.o astr.o amath.o autil.o dyna.o
	$(AR) -crs _brt.a $^

diss: dyna.o
	m68k-amigaos-objdump -S -d -x dyna.o

%.asm: %.c
	$(CC) -S -Wall -Werror -march=68000 -mcpu=68000 $(CPPFLAGS) $< -o $@

clean:
	rm -f *.o alibtest _brt.a *.sym

