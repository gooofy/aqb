include ../../../config.mk

.PHONY: clean all diss

all: _brt.a startup.o _brt.sym

%.o : %.c
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) $< -o $@

_brt.sym : _brt.bas
	$(AQB) -d none -S _brt.sym $<
	pushd .. && rm -f _brt.sym && ln -s _brt/_brt.sym && popd

%.o : %.s
	$(AS_AMIGAOS) $(ASFLAGS_AMIGAOS) $< -o $@

_brt.a: cstartup.o astr.o amath.o autil.o dyna.o data.o
	$(AR_AMIGAOS) -crs _brt.a $^

diss: dyna.o
	$(OBJDUMP_AMIGAOS) -S -d -x dyna.o

%.asm: %.c
	$(CC_AMIGAOS) -S -Wall -Werror -march=68000 -mcpu=68000 $< -o $@

clean:
	rm -f *.o alibtest _brt.a *.sym

