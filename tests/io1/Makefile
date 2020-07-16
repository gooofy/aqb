
AQB = ../../src/compiler/aqb
AS = m68k-amigaos-as
LD = m68k-amigaos-ld

AQBFLAGS = -L ../../src/lib

.PHONY:	clean all

all:	io1.bin

clean:
	rm -f *.s *.bin *.o *.sym

%.o:	%.s
	$(AS) $< -o $@

io1.s:	io1.bas
	$(AQB) $(AQBFLAGS) io1.bas

io1.bin:	io1.o
	$(LD) ../../src/lib/_brt/startup.o io1.o ../../src/lib/_brt/_brt.a ../../src/lib/_aqb/_aqb.a -o io1.bin
	cp io1.bin /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/
	cp io1.bin /home/guenter/media/emu/amiga/a500/hdd/system/x/



