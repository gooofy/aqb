include ../../../config.mk

.PHONY: clean all

all: _aqb.a _aqb.sym

%.o : %.c
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) $< -o $@

%.o : %.s
	$(AS_AMIGAOS) $(ASFLAGS_AMIGAOS) $< -o $@

_aqb.sym : _aqb.bas
	$(AQB) -L .. -d _brt -s _aqb.sym $<
	pushd .. && rm -f _aqb.sym _aqb.a && ln -s _aqb/_aqb.sym && ln -s _aqb/_aqb.a && popd
	cp $< $(DISTDIR)/libsrc
	cp $@ $(DISTDIR)/lib

_aqb.a: awindow.o _aqb.o aio.o atimer.o autil.o
	$(AR_AMIGAOS) -crs _aqb.a $^
	pushd .. && rm -f _aqb.a && ln -s _aqb/_aqb.a && popd
	cp $@ $(DISTDIR)/lib

clean:
	rm -f *.o _aqb.a *.sym

