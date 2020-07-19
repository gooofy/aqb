
AQB = ../../src/compiler/aqb
AS = m68k-amigaos-as
LD = m68k-amigaos-ld

.PHONY:	clean all

all:	gfx1.bin

clean:
	rm -f *.s *.bin *.o *.sym

%.o:	%.s
	$(AS) $< -o $@

gfx1.s:	gfx1.bas
	$(AQB) -L ../../src/lib gfx1.bas

gfx1.bin:	gfx1.o
	$(LD) ../../src/lib/_brt/startup.o gfx1.o ../../src/lib/_brt/_brt.a ../../src/lib/_aqb/_aqb.a -o gfx1.bin
	cp gfx1.bin /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/
	cp gfx1.bin /home/guenter/media/emu/amiga/a500/hdd/system/x/



