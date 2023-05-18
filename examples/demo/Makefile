include ../../config.mk

AQBFLAGS     = -L ../../src/lib

SRCS = gfx1.bas gfx2.bas 3dplot.bas hand.bas tetris.bas mandelbrot.bas banana.bas music.bas bezier.bas

ASMDIR_AMIGAOS_DEMO = aqb_demo/asm
OBJDIR_AMIGAOS_DEMO = aqb_demo/obj
BINDIR_AMIGAOS_DEMO = aqb_demo

ASMS = $(patsubst %,${EMUDIR}/${ASMDIR_AMIGAOS_DEMO}/%.s,$(basename $(SRCS)))
OBJS = $(patsubst %,${EMUDIR}/${OBJDIR_AMIGAOS_DEMO}/%.o,$(basename $(SRCS)))
BINS = $(patsubst %,${EMUDIR}/${BINDIR_AMIGAOS_DEMO}/%,$(basename $(SRCS)))

.PHONY:	clean all dirs

all:	dirs $(BINS)

$(ASMS) : $(EMUDIR)/$(ASMDIR_AMIGAOS_DEMO)/%.s : %.bas
	$(AQB) $(AQBFLAGS) -a $@ $<

$(OBJS) : $(EMUDIR)/$(OBJDIR_AMIGAOS_DEMO)/%.o : $(EMUDIR)/$(ASMDIR_AMIGAOS_DEMO)/%.s
	$(AS_AMIGAOS) $(ASFLAGS_AMIGAOS) $< -o $@

$(BINS) : $(EMUDIR)/$(BINDIR_AMIGAOS_DEMO)/% : $(EMUDIR)/$(OBJDIR_AMIGAOS_DEMO)/%.o
	$(LD_AMIGAOS) ../../src/lib/_brt/startup.o $< ../../src/lib/IFFSupport/IFFSupport.a ../../src/lib/_aqb/_aqb.a ../../src/lib/_brt/_brt.a -o $@
	$(STRIP_AMIGAOS) $@

clean:
	rm -rf $(EMUDIR)/$(ASMDIR_AMIGAOS_DEMO)
	rm -rf $(EMUDIR)/$(OBJDIR_AMIGAOS_DEMO)
	rm -rf $(EMUDIR)/$(BINDIR_AMIGAOS_DEMO)

dirs:
	mkdir -p $(EMUDIR)/$(ASMDIR_AMIGAOS_DEMO)
	mkdir -p $(EMUDIR)/$(OBJDIR_AMIGAOS_DEMO)
	mkdir -p $(EMUDIR)/$(BINDIR_AMIGAOS_DEMO)

