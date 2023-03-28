include ../../../config.mk

.PHONY: clean all

all: GadToolsSupport.a GadToolsSupport.sym

%.o : %.c
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) -I../_aqb $< -o $@

GadToolsSupport.sym : GadToolsSupport.bas
	$(AQB) -L .. -d _aqb -s GadToolsSupport.sym $<
	pushd .. && rm -f GadToolsSupport.sym GadToolsSupport.a && ln -s GadToolsSupport/GadToolsSupport.sym && ln -s GadToolsSupport/GadToolsSupport.a && popd
	cp $< $(DISTDIR)/libsrc
	cp $@ $(DISTDIR)/lib
	rm -f $(DISTDIR)/lib/*.uaem

GadToolsSupport.a: GadToolsSupport.o gtbutton.o gtcheckbox.o gtslider.o gttext.o gtscroller.o gtstring.o gtinteger.o gtnumber.o gtmx.o gtcycle.o
	$(AR_AMIGAOS) -crs GadToolsSupport.a $^
	pushd .. && rm -f GadToolsSupport.a && ln -s GadToolsSupport/GadToolsSupport.a && popd
	cp $@ $(DISTDIR)/lib

%.asm: %.c
	$(CC_AMIGAOS) -S $(CFLAGS_AMIGAOS) -I../_aqb $< -o $@

clean:
	rm -f *.o GadToolsSupport.a *.sym *.asm aqb.log

