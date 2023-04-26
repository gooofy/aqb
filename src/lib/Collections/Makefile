include ../../../config.mk

.PHONY: clean all diss

all: stub.c Collections.a Collections.sym

%.o : %.c
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) $< -o $@

Collections.sym : Collections.bas
	$(AQB) -L .. -d _brt -s $@ $<
	pushd .. && rm -f Collections.sym && ln -s Collections/Collections.sym && popd
	cp $< $(DISTDIR)/libsrc
	cp $@ $(DISTDIR)/lib
	rm -f $(DISTDIR)/lib/*.uaem

stub.c: Collections.bas
	$(AQB) -L .. -d _brt -S $@ $<

%.o : %.s
	$(AS_AMIGAOS) $(ASFLAGS_AMIGAOS) $< -o $@

Collections.a: Collections.o ArrayList.o ArrayListEnumerator.o ExecList.o
	$(AR_AMIGAOS) -crs Collections.a $^
	pushd .. && rm -f Collections.a && ln -s Collections/Collections.a && popd
	cp $@ $(DISTDIR)/lib

# diss: startup.o
# 	$(OBJDUMP_AMIGAOS) -S -d -x startup.o

%.asm: %.c
	$(CC_AMIGAOS) -S $(CFLAGS_AMIGAOS) $< -o $@

clean:
	rm -f *.o Collections.a *.sym *.asm aqb.log stub.c

