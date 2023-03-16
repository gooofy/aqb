include ../../../config.mk

.PHONY: clean all diss

all: startup.o

startup.o : startup.s
	$(AS_AMIGAOS) $(ASFLAGS_AMIGAOS) $< -o $@
	#cp startup.o $(DISTDIR)/lib

diss: startup.o
	$(OBJDUMP_AMIGAOS) -S -d -x startup.o

clean:
	rm -f *.o

