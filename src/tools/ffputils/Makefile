
CFLAGS = -O -Wall -Werror
ASFLAGS =
LDFLAGS = -lm

CC = gcc
AS = as
LD = ld
AR = ar

.PHONY: clean all

all: ffputils

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

alibtest: ffputils.o
	$(LD) $(LDFLAGS) $^ -o $@

clean:
	rm -f *.o ffputils

