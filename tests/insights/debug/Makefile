include ../../../config.mk

.PHONY:	run clean

all:	foo.hunk

foo.hunk:	foo.bas
	

run:
	$(LXA) -r $(LXA_ROM) "$(WORKDIR)/foo.hunk"

clean:
	rm -f "$(WORKDIR)/foo.hunk"

