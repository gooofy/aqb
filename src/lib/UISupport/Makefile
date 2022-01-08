include ../../../config.mk

.PHONY: clean all

all: UISupport.a UISupport.sym

%.o : %.c
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) -I../_aqb $< -o $@

UISupport.sym : UISupport.bas
	$(AQB) -L .. -d _aqb -s UISupport.sym $<
	pushd .. && rm -f UISupport.sym UISupport.a && ln -s UISupport/UISupport.sym && ln -s UISupport/UISupport.a && popd
	cp $< $(DISTDIR)/libsrc
	cp $@ $(DISTDIR)/lib
	rm -f $(DISTDIR)/lib/*.uaem

UISupport.a: UISupport.o
	$(AR_AMIGAOS) -crs UISupport.a $^
	pushd .. && rm -f UISupport.a && ln -s UISupport/UISupport.a && popd
	cp $@ $(DISTDIR)/lib

%.asm: %.c
	$(CC_AMIGAOS) -S $(CFLAGS_AMIGAOS) -I../_aqb $< -o $@

clean:
	rm -f *.o UISupport.a *.sym *.asm aqb.log

