include ../../../config.mk

.PHONY: clean all

all: OSGadTools.sym OSGadTools.a

%.o : %.c
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) -I../_aqb $< -o $@

OSGadTools.sym : OSGadTools.bas
	$(AQB) -L .. -d _aqb -s OSGadTools.sym $<
	pushd .. && rm -f OSGadTools.sym OSGadTools.a && ln -s OSGadTools/OSGadTools.sym && popd
	cp $< $(DISTDIR)/libsrc
	cp $@ $(DISTDIR)/lib
	rm -f $(DISTDIR)/lib/*.uaem

OSGadTools.a: OSGadTools.o
	$(AR_AMIGAOS) -crs OSGadTools.a $^
	pushd .. && rm -f OSGadTools.a && ln -s OSGadTools/OSGadTools.a && popd
	cp $@ $(DISTDIR)/lib

%.asm: %.c
	$(CC_AMIGAOS) -S $(CFLAGS_AMIGAOS) -I../_aqb $< -o $@

clean:
	rm -f *.o OSGadTools.a *.sym *.asm aqb.log

