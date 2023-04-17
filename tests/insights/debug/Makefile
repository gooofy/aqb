include ../../../config.mk

EXAMPLE=foo
# AQBFLAGS="-v"
AQBFLAGS=""
AQBLIBS=../../../src/lib
# AQBRT=_aqb
AQBRT=_brt
ASMProsrctmp='foo.asm'
ASMProsrc="${AMIGA_X}/foo.asm"

.PHONY:	run clean

all:	foo.hunk

foo.hunk:	foo.bas
	$(AQB) -L $(AQBLIBS) -d $(AQBRT) -A $(ASMProsrctmp) -o foo.hunk $(EXAMPLE).bas || exit 1
	cp foo.hunk ${AMIGA_X}

run:
	$(LXA) -r $(LXA_ROM) "$(WORKDIR)/foo.hunk"

clean:
	rm -f "$(WORKDIR)/foo.hunk"
	rm -f aqb.log $(ASMProsrctmp) foo.hunk foo.s $(ASMProsrc)

