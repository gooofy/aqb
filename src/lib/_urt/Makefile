include ../../../config.mk

.PHONY: clean all diss startup

#all: _urt_stub.c startup.o _urt.a _urt.sym
#all: _urt.sym
all: startup _urt.a

%.o : %.c
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) $< -o $@

_urt.sym _urt.o _urt.s : _urt.cs
	$(ACS) _urt $<
	#$(ACS) -d none -N -E -s _urt.sym -a _urt.asm -p _urt.o $<
	#pushd .. && rm -f _urt.sym && ln -s _urt/_urt.sym && popd
	#cp $< $(DISTDIR)/libsrc
	#cp $@ $(DISTDIR)/lib
	#rm -f $(DISTDIR)/lib/*.uaem

%.o : %.s
	$(AS_AMIGAOS) $(ASFLAGS_AMIGAOS) $< -o $@

%.o : %.S
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) $< -o $@

#_urt.a: cstartup.o _urt.o astr.o cstring.o amath.o autil.o carray.o data.o dprintf.o aio.o oop.o memmove.o bcopy.o gc.o
_urt.a: cstartup.o astr.o amath.o autil.o dprintf.o memmove.o bcopy.o gc.o
	$(AR_AMIGAOS) -crs _urt.a $^
	pushd .. && rm -f _urt.a && ln -s _urt/_urt.a && popd
	cp $@ $(DISTDIR)/lib
	cp startup.o $(DISTDIR)/lib

startup:	startup.o
	pushd .. && rm -f startup.o && ln -s _urt/startup.o && popd

diss: startup.o
	$(OBJDUMP_AMIGAOS) -S -d -x startup.o

%.asm: %.c
	$(CC_AMIGAOS) -S $(CFLAGS_AMIGAOS) $< -o $@

_urt_stub.c: _urt.cs
	$(ACS) -L .. -N -E -d none -S $@ $<

clean:
	rm -f *.o alibtest _urt.a *.sym *.asm aqb.log _urt_stub.c

