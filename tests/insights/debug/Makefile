include ../../../config.mk

EXAMPLE=foo
# ACSFLAGS="-v"
ACSFLAGS=""
ACSLIBS=../../../src/lib
# ACSRT=_aqb
ACSRT=_brt
ASMProsrctmp='foo.asm'
ASMProsrc="${AMIGA_X}/foo.asm"

.PHONY:	run clean

all:	foo.hunk $(EMUDIR)/foo.hunk

$(EXAMPLE).hunk:	$(EXAMPLE).cs
	$(ACS) -L $(ACSLIBS) -A $(ASMProsrctmp) -o $(EXAMPLE).hunk $(EXAMPLE) $(EXAMPLE).cs || exit 1
	# $(ACS) -A $(ASMProsrctmp) -p $(EXAMPLE).o $(EXAMPLE) $(EXAMPLE).cs || exit 1
	# $(ACS) -L $(ACSLIBS) -d $(ACSRT) -A $(ASMProsrctmp) -o foo.hunk $(EXAMPLE).cs || exit 1

$(EMUDIR)/foo.hunk: foo.hunk
	cp foo.hunk $(EMUDIR)

run: $(EMUDIR)/foo.hunk
	$(LXA) -r $(LXA_ROM) "$(WORKDIR)/foo.hunk"

debug: $(EMUDIR)/foo.hunk
	$(LXA) -b __aqb_main -r $(LXA_ROM) "$(WORKDIR)/foo.hunk"

clean:
	rm -f "$(EMUDIR)/foo.hunk"
	rm -f aqb.log $(ASMProsrctmp) foo.hunk foo.s $(ASMProsrc)

