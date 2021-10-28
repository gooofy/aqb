include ../../../config.mk

.PHONY: clean all

all: AnimSupport.a AnimSupport.sym

%.o : %.c
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) -I../_aqb $< -o $@

AnimSupport.sym : AnimSupport.bas
	$(AQB) -L .. -d _aqb -s AnimSupport.sym $<
	pushd .. && rm -f AnimSupport.sym AnimSupport.a && ln -s AnimSupport/AnimSupport.sym && ln -s AnimSupport/AnimSupport.a && popd
	cp $< $(DISTDIR)/libsrc
	cp $@ $(DISTDIR)/lib
	rm -f $(DISTDIR)/lib/*.uaem

AnimSupport.a: AnimSupport.o
	$(AR_AMIGAOS) -crs AnimSupport.a $^
	pushd .. && rm -f AnimSupport.a && ln -s AnimSupport/AnimSupport.a && popd
	cp $@ $(DISTDIR)/lib

%.asm: %.c
	$(CC_AMIGAOS) -S $(CFLAGS_AMIGAOS) -I../_aqb $< -o $@

clean:
	rm -f *.o AnimSupport.a *.sym *.asm aqb.log

