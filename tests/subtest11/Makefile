
AQB = ../../target/x86_64-linux/bin/aqb
AS = m68k-amigaos-as
LD = m68k-amigaos-ld

AQBFLAGS = -L ../../src/lib

.PHONY:	clean all

all:	subtest11.bin

clean:
	rm -f *.s *.bin *.o *.sym

%.o:	%.s
	$(AS) $< -o $@

subtest11.s:	subtest11.bas
	$(AQB) $(AQBFLAGS) subtest11.bas

subtest11.bin:	subtest11.o
	$(LD) ../../src/lib/_brt/startup.o subtest11.o ../../src/lib/_brt/_brt.a ../../src/lib/_aqb/_aqb.a -o subtest11.bin
	cp subtest11.bin /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/
	cp subtest11.bin /home/guenter/media/emu/amiga/a500/hdd/system/x/



