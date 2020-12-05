
AQB = ../../target/x86_64-linux/bin/aqb
AS = m68k-amigaos-as
LD = m68k-amigaos-ld

.PHONY:	clean all

all:	gfx1.bin gfx2.bin 3dplot.bin hand.bin tetris.bin

clean:
	rm -f *.s *.bin *.o *.sym

%.o:	%.s
	$(AS) $< -o $@

%.s:	%.bas
	$(AQB) -L ../../src/lib $<

%.bin:	%.o
	$(LD) ../../src/lib/_brt/startup.o $< ../../src/lib/_brt/_brt.a ../../src/lib/_aqb/_aqb.a -o $@
	cp $@ /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/
	cp $@ /home/guenter/media/emu/amiga/a500/hdd/system/x/


