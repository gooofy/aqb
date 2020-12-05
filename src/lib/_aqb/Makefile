
CFLAGS = -O3 -Wall -Werror -march=68000 -mcpu=68000
ASFLAGS = -march=68000 -mcpu=68000

CC = m68k-amigaos-gcc
AS = m68k-amigaos-as
LD = m68k-amigaos-ld
AR = m68k-amigaos-ar
AQB = ../../../target/x86_64-linux/bin/aqb

.PHONY: clean all

all: _aqb.a _aqb.sym

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.o : %.s
	$(AS) $(ASFLAGS) $< -o $@

_aqb.sym : _aqb.bas
	$(AQB) -L .. -d _brt -S $<
	pushd .. && rm -f _aqb.sym && ln -s _aqb/_aqb.sym && popd

_aqb.a: awindow.o _aqb.o aio.o
	$(AR) -crs _aqb.a $^

clean:
	rm -f *.o _aqb.a *.sym

