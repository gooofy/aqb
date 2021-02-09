include ../../../config.mk

.PHONY: clean all

all: hunk

%.o : %.c
	$(CC_LINUX) -c $(CFLAGS_LINUX) $< -o $@

hunk: main.o
	$(CC_LINUX) $(CFLAGS_LINUX) $(LDFLAGS_LINUX) -o $@ $^

clean:
	rm -f *.o hunk

