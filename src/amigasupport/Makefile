include ../../config.mk

.PHONY: clean all

all: amigasupport.a

%.o : %.c
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) $< -o $@

%.o : %.s
	$(AS_AMIGAOS) $(ASFLAGS_AMIGAOS) $< -o $@

amigasupport.a: amigasupport.o
	$(AR_AMIGAOS) -crs amigasupport.a $^

clean:
	rm -f *.o amigasupport.a

