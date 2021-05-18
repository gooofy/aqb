include ../../config.mk

AQBFLAGS     = -L ../../src/lib
AQBFLAGS_OPT = -O -L ../../src/lib

SRCS = gfx1.bas gfx2.bas 3dplot.bas hand.bas tetris.bas mandelbrot.bas

ASMDIR_AMIGAOS_DEMO = aqb_demo/asm
OBJDIR_AMIGAOS_DEMO = aqb_demo/obj
BINDIR_AMIGAOS_DEMO = aqb_demo

ASMDIR_AMIGAOS_DEMO_OPT = aqb_demo_opt/asm
OBJDIR_AMIGAOS_DEMO_OPT = aqb_demo_opt/obj
BINDIR_AMIGAOS_DEMO_OPT = aqb_demo_opt

ASMS = $(patsubst %,${EMUDIR}/${ASMDIR_AMIGAOS_DEMO}/%.s,$(basename $(SRCS)))
OBJS = $(patsubst %,${EMUDIR}/${OBJDIR_AMIGAOS_DEMO}/%.o,$(basename $(SRCS)))
BINS = $(patsubst %,${EMUDIR}/${BINDIR_AMIGAOS_DEMO}/%,$(basename $(SRCS)))

ASMS_OPT = $(patsubst %,${EMUDIR}/${ASMDIR_AMIGAOS_DEMO_OPT}/%.s,$(basename $(SRCS)))
OBJS_OPT = $(patsubst %,${EMUDIR}/${OBJDIR_AMIGAOS_DEMO_OPT}/%.o,$(basename $(SRCS)))
BINS_OPT = $(patsubst %,${EMUDIR}/${BINDIR_AMIGAOS_DEMO_OPT}/%,$(basename $(SRCS)))

.PHONY:	clean all dirs

all:	dirs $(BINS) $(BINS_OPT)

$(ASMS) : $(EMUDIR)/$(ASMDIR_AMIGAOS_DEMO)/%.s : %.bas
	$(AQB) $(AQBFLAGS) -a $@ $<

$(OBJS) : $(EMUDIR)/$(OBJDIR_AMIGAOS_DEMO)/%.o : $(EMUDIR)/$(ASMDIR_AMIGAOS_DEMO)/%.s
	$(AS_AMIGAOS) $(ASFLAGS_AMIGAOS) $< -o $@

$(BINS) : $(EMUDIR)/$(BINDIR_AMIGAOS_DEMO)/% : $(EMUDIR)/$(OBJDIR_AMIGAOS_DEMO)/%.o
	$(LD_AMIGAOS) ../../src/lib/_brt/startup.o $< ../../src/lib/_brt/_brt.a ../../src/lib/_aqb/_aqb.a -o $@
	$(STRIP_AMIGAOS) $@

$(ASMS_OPT) : $(EMUDIR)/$(ASMDIR_AMIGAOS_DEMO_OPT)/%.s : %.bas
	$(AQB) $(AQBFLAGS_OPT) -a $@ $<

$(OBJS_OPT) : $(EMUDIR)/$(OBJDIR_AMIGAOS_DEMO_OPT)/%.o : $(EMUDIR)/$(ASMDIR_AMIGAOS_DEMO_OPT)/%.s
	$(AS_AMIGAOS) $(ASFLAGS_AMIGAOS) $< -o $@

$(BINS_OPT) : $(EMUDIR)/$(BINDIR_AMIGAOS_DEMO_OPT)/% : $(EMUDIR)/$(OBJDIR_AMIGAOS_DEMO_OPT)/%.o
	$(LD_AMIGAOS) ../../src/lib/_brt/startup.o $< ../../src/lib/_brt/_brt.a ../../src/lib/_aqb/_aqb.a -o $@
	$(STRIP_AMIGAOS) $@

clean:
	rm -rf $(EMUDIR)/$(ASMDIR_AMIGAOS_DEMO)
	rm -rf $(EMUDIR)/$(OBJDIR_AMIGAOS_DEMO)
	rm -rf $(EMUDIR)/$(BINDIR_AMIGAOS_DEMO)
	rm -rf $(EMUDIR)/$(ASMDIR_AMIGAOS_DEMO_OPT)
	rm -rf $(EMUDIR)/$(OBJDIR_AMIGAOS_DEMO_OPT)
	rm -rf $(EMUDIR)/$(BINDIR_AMIGAOS_DEMO_OPT)

dirs:
	mkdir -p $(EMUDIR)/$(ASMDIR_AMIGAOS_DEMO)
	mkdir -p $(EMUDIR)/$(OBJDIR_AMIGAOS_DEMO)
	mkdir -p $(EMUDIR)/$(BINDIR_AMIGAOS_DEMO)
	mkdir -p $(EMUDIR)/$(ASMDIR_AMIGAOS_DEMO_OPT)
	mkdir -p $(EMUDIR)/$(OBJDIR_AMIGAOS_DEMO_OPT)
	mkdir -p $(EMUDIR)/$(BINDIR_AMIGAOS_DEMO_OPT)

