include ../config.mk

AQBFLAGS     = -L ../src/lib

SRCS = GadToolsTutorial1.bas GadToolsTutorial2.bas GadToolsTutorial3.bas GadToolsTutorial4.bas \
	   GadToolsTutorial5.bas GadToolsTutorial6.bas \
	   BitmapDraw.bas BOBDemo1.bas BOBDemo2.bas CtrlC.bas CustomPointer.bas \
	   Debug.bas FontTutorial.bas \
	   ILBMBitmap.bas ILBMShow.bas \
	   SoundDemo1.bas SoundDemo2.bas SoundDemo3.bas \
	   SpriteDemo1.bas SpriteDemo2.bas SpriteDemo3.bas \
	   TagListTutorial.bas \
	   MenuTutorial1.bas

ASMDIR_AMIGAOS_TUTORIAL = aqb_tutorial/asm
OBJDIR_AMIGAOS_TUTORIAL = aqb_tutorial/obj
BINDIR_AMIGAOS_TUTORIAL = aqb_tutorial

ASMS = $(patsubst %,${EMUDIR}/${ASMDIR_AMIGAOS_TUTORIAL}/%.s,$(basename $(SRCS)))
OBJS = $(patsubst %,${EMUDIR}/${OBJDIR_AMIGAOS_TUTORIAL}/%.o,$(basename $(SRCS)))
BINS = $(patsubst %,${EMUDIR}/${BINDIR_AMIGAOS_TUTORIAL}/%,$(basename $(SRCS)))

.PHONY:	clean all dirs

all:	dirs $(BINS)

$(ASMS) : $(EMUDIR)/$(ASMDIR_AMIGAOS_TUTORIAL)/%.s : %.bas
	$(AQB) $(AQBFLAGS) -a $@ $<

$(OBJS) : $(EMUDIR)/$(OBJDIR_AMIGAOS_TUTORIAL)/%.o : $(EMUDIR)/$(ASMDIR_AMIGAOS_TUTORIAL)/%.s
	$(AS_AMIGAOS) $(ASFLAGS_AMIGAOS) $< -o $@

$(BINS) : $(EMUDIR)/$(BINDIR_AMIGAOS_TUTORIAL)/% : $(EMUDIR)/$(OBJDIR_AMIGAOS_TUTORIAL)/%.o
	$(LD_AMIGAOS) ../src/lib/_brt/startup.o $< ../src/lib/AnimSupport.a ../src/lib/GadToolsSupport.a ../src/lib/IntuiSupport.a ../src/lib/Collections.a ../src/lib/IFFSupport.a ../src/lib/OSGadTools.a ../src/lib/_brt/_brt.a ../src/lib/_aqb/_aqb.a -o $@
	$(STRIP_AMIGAOS) $@

clean:
	rm -rf $(EMUDIR)/$(ASMDIR_AMIGAOS_TUTORIAL)
	rm -rf $(EMUDIR)/$(OBJDIR_AMIGAOS_TUTORIAL)
	rm -rf $(EMUDIR)/$(BINDIR_AMIGAOS_TUTORIAL)

dirs:
	mkdir -p $(EMUDIR)/$(ASMDIR_AMIGAOS_TUTORIAL)
	mkdir -p $(EMUDIR)/$(OBJDIR_AMIGAOS_TUTORIAL)
	mkdir -p $(EMUDIR)/$(BINDIR_AMIGAOS_TUTORIAL)
	cp -r ../dist/amiga/8svx $(EMUDIR)/$(BINDIR_AMIGAOS_TUTORIAL)/
	cp -r ../dist/amiga/imgs $(EMUDIR)/$(BINDIR_AMIGAOS_TUTORIAL)/

