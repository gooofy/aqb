include ../../../config.mk

.PHONY: clean all diss

all: startup.o _brt.a _brt.sym

%.o : %.c
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) $< -o $@

_brt.sym : _brt.bas
	$(AQB) -d none -s $@ $<
	pushd .. && rm -f _brt.sym && ln -s _brt/_brt.sym && popd
	cp $< $(DISTDIR)/libsrc
	cp $@ $(DISTDIR)/lib
	rm -f $(DISTDIR)/lib/*.uaem

%.o : %.s
	$(AS_AMIGAOS) $(ASFLAGS_AMIGAOS) $< -o $@

_brt.a: cstartup.o astr.o amath.o autil.o dyna.o data.o
	$(AR_AMIGAOS) -crs _brt.a $^
	pushd .. && rm -f startup.o _brt.a && ln -s _brt/startup.o && ln -s _brt/_brt.a && popd
	cp $@ $(DISTDIR)/lib
	cp startup.o $(DISTDIR)/lib

diss: dyna.o
	$(OBJDUMP_AMIGAOS) -S -d -x dyna.o

%.asm: %.c
	$(CC_AMIGAOS) -S $(CFLAGS_AMIGAOS) $< -o $@

clean:
	rm -f *.o alibtest _brt.a *.sym

