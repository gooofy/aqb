include ../../../config.mk

.PHONY: clean all

.PRECIOUS:	%.s

all: minbrt.asm minbrt.o prolog.o

%.s: %.c
	$(CC_AMIGAOS) -S -Wall -Werror -march=68000 -mcpu=68000 $< -o $@

%.o: %.s
	$(AS_AMIGAOS) $< -o $@

%.asm: %.s
	./as2mot.py $< > $@

clean:
	rm -f minbrt.asm minbrt.s prolog.o

