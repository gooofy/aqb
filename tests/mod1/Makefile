
AQB = ../../src/compiler/aqb
AS = m68k-amigaos-as
LD = m68k-amigaos-ld

.PHONY:	clean all

all:	mod1.bin

clean:
	rm -f *.s *.bin *.o *.sym

foo.sym:	foo.bas
	$(AQB) -s foo.bas

foo.s:		foo.sym
	noop

bar.sym:	bar.bas
	$(AQB) -s bar.bas

bar.s:		bar.sym
	noop

%.o:	%.s
	$(AS) $< -o $@

mod1.s:	mod1.bas foo.sym bar.sym
	$(AQB) mod1.bas

mod1.bin:	mod1.o foo.o bar.o
	$(LD) ../../src/alib/startup.o mod1.o foo.o bar.o ../../src/alib/_alib.a -o mod1.bin
	cp mod1.bin /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/
	cp mod1.bin /home/guenter/media/emu/amiga/a500/hdd/system/x/



