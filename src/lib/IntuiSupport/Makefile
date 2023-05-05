include ../../../config.mk

.PHONY: clean all

all: stub.c IntuiSupport.a IntuiSupport.sym

%.o : %.c
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) -I../_aqb $< -o $@

IntuiSupport.sym : IntuiSupport.bas
	$(AQB) -L .. -d _aqb -s IntuiSupport.sym $<
	pushd .. && rm -f IntuiSupport.sym IntuiSupport.a && ln -s IntuiSupport/IntuiSupport.sym && ln -s IntuiSupport/IntuiSupport.a && popd
	cp $< $(DISTDIR)/libsrc
	cp $@ $(DISTDIR)/lib
	rm -f $(DISTDIR)/lib/*.uaem

IntuiSupport.a: IntuiSupport.o menu.o menuitemtext.o menuitem.o
	$(AR_AMIGAOS) -crs IntuiSupport.a $^
	pushd .. && rm -f IntuiSupport.a && ln -s IntuiSupport/IntuiSupport.a && popd
	cp $@ $(DISTDIR)/lib

stub.c: IntuiSupport.bas
	$(AQB) -L .. -d _brt -S $@ $<

%.asm: %.c
	$(CC_AMIGAOS) -S $(CFLAGS_AMIGAOS) -I../_aqb $< -o $@

clean:
	rm -f *.o IntuiSupport.a *.sym *.asm aqb.log

