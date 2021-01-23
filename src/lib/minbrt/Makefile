include ../../../config.mk

.PHONY: clean all

.PRECIOUS:	%.s

all: minbrt.asm

%.s: %.c
	$(CC_AMIGAOS) -S -Wall -Werror -march=68000 -mcpu=68000 $< -o $@

%.asm: %.s
	./as2mot.py $< > $@

clean:
	rm -f minbrt.asm minbrt.s

