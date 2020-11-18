CFLAGS = -O3 -Wall -march=68000 -mcpu=68000
ASFLAGS = -march=68000 -mcpu=68000
AQBFLAGS = -L ../src/lib -d _brt

CC = m68k-amigaos-gcc
AS = m68k-amigaos-as
LD = m68k-amigaos-ld
AR = m68k-amigaos-ar
AQB = ../src/compiler/aqb
STRIP = m68k-amigaos-strip

.PHONY: clean all mod1 io1 subtest11

all: cond1.bin cond3.bin cond4.bin	\
	const1.bin const2.bin const3.bin constprop.bin \
	ctrl1.bin ctrl2.bin ctrl3.bin ctrl4.bin ctrl5.bin ctrl6.bin \
	dynmem1.bin \
	exp1.bin exp2.bin \
	extcall2.bin \
	loop10.bin loop1.bin loop2.bin loop3.bin loop4.bin loop5.bin loop6.bin loop7.bin loop8.bin loop9.bin \
	opbool.bin opbyte.bin opint.bin oplong.bin opsingle.bin opubyte.bin opuint.bin opulong.bin \
	oop1.bin oop2.bin oop3.bin oop4.bin \
	scopes1.bin scopes2.bin scopes3.bin \
	string1.bin string2.bin \
	subtest1.bin subtest2.bin subtest3.bin subtest4.bin subtest5.bin \
	subtest6.bin subtest7.bin subtest8.bin subtest9.bin \
	subtest10.bin subtest11 subtest12.bin subtest13.bin subtest14.bin subtest15.bin subtest16.bin \
	subtest17.bin subtest18.bin subtest19.bin subtest20.bin subtest21.bin subtest22.bin \
	types1.bin types2.bin types3.bin types4.bin types5.bin types6.bin types7.bin \
	var1.bin var2.bin \
	mod1 io1 \
	sarray1.bin sarray2.bin sarray3.bin sarray4.bin sarray5.bin sarray6.bin \
	darray1.bin darray2.bin darray3.bin darray4.bin darray5.bin darray6.bin darray7.bin darray8.bin

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.s : %.bas
	$(AQB) $(AQBFLAGS) $<

%.o : %.s
	$(AS) $(ASFLAGS) $< -o $@

%.bin : %.o
	$(LD) ../src/lib/_brt/startup.o $< ../src/lib/_brt/_brt.a -o $@
	$(STRIP) $@
	cp $@ /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/
	cp $@ /home/guenter/media/emu/amiga/a500/hdd/system/x/

mod1:
	cd mod1 ; make all
io1:
	cd io1 ; make all
subtest11:
	cd subtest11 ; make all

clean:
	rm -f *.o *.s *.bin *.sym
	cd mod1 ; make clean
	cd io1 ; make clean
	cd subtest11 ; make clean
