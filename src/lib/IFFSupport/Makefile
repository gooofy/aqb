include ../../../config.mk

.PHONY: clean all

all: IFFSupport.a IFFSupport.sym

%.o : %.c
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) -I../_aqb $< -o $@

IFFSupport.sym : IFFSupport.bas
	$(AQB) -L .. -d _aqb -s IFFSupport.sym $<
	pushd .. && rm -f IFFSupport.sym IFFSupport.a && ln -s IFFSupport/IFFSupport.sym && ln -s IFFSupport/IFFSupport.a && popd
	cp $< $(DISTDIR)/libsrc
	cp $@ $(DISTDIR)/lib
	rm -f $(DISTDIR)/lib/*.uaem

IFFSupport.a: IFFSupport.o
	$(AR_AMIGAOS) -crs IFFSupport.a $^
	pushd .. && rm -f IFFSupport.a && ln -s IFFSupport/IFFSupport.a && popd
	cp $@ $(DISTDIR)/lib

%.asm: %.c
	$(CC_AMIGAOS) -S $(CFLAGS_AMIGAOS) -I../_aqb $< -o $@

clean:
	rm -f *.o IFFSupport.a *.sym *.asm

