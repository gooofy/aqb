
CFLAGS = -I.. -O -Wall -march=68000 -mcpu=68000
ASFLAGS = -march=68000 -mcpu=68000

CC = m68k-amigaos-gcc
AS = m68k-amigaos-as
LD = m68k-amigaos-ld
AR = m68k-amigaos-ar

.PHONY: clean all

all: win1test.bin timer1test.bin gadget1test.bin gadget1test.s ptr1test.s ptr1test.bin

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

win1test.bin: ../startup.o ../alibtest.o win1test.o ../_alib.a
	$(LD) $^ -o $@
	cp $@ /home/guenter/media/emu/amiga/a500/hdd/system/x/

timer1test.bin: ../startup.o ../alibtest.o timer1test.o ../_alib.a
	$(LD) $^ -o $@
	cp $@ /home/guenter/media/emu/amiga/a500/hdd/system/x/

ptr1test.bin: ../startup.o ../alibtest.o ptr1test.o ../_alib.a
	$(LD) $^ -o $@
	cp $@ /home/guenter/media/emu/amiga/a500/hdd/system/x/

gadget1test.bin: ../startup.o ../alibtest.o gadget1test.o ../_alib.a
	$(LD) $^ -o $@
	cp $@ /home/guenter/media/emu/amiga/a500/hdd/system/x/

gadget1test.s:	gadget1test.c
	$(CC) -S $(CFLAGS) $(CPPFLAGS) $< -o $@

ptr1test.s:	ptr1test.c
	$(CC) -S $(CFLAGS) $(CPPFLAGS) $< -o $@

clean:
	rm -f *.o *.bin

