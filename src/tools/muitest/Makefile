include ../../../config.mk

CFLAGS_MUI = -I/home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/mui/SDK/MUI/C/include
CFLAGS_AMIGAOS2   = -O3 -march=68000 -mcpu=68000 -mcrt=nix20 -I$(SELF_DIR)/src/amigasupport

SRCS = 	muitest.c

OBJS_AMIGAOS := $(patsubst %,${OBJDIR_AMIGAOS}/%.o,$(basename $(SRCS)))

.PHONY: clean all
all:	$(BINDIR_AMIGAOS)/muitest

clean:
	rm -f $(OBJDIR_AMIGAOS)/*.o $(BINDIR_AMIGAOS)/muitest muitest.asm
	rm -rf $(DEPDIR_AMIGAOS)

$(OBJDIR_AMIGAOS)/%.o : %.c $(DEPDIR_AMIGAOS)/%.d | $(DEPDIR_AMIGAOS)
	$(CC_AMIGAOS) -c -g $(CFLAGS_AMIGAOS) $(CFLAGS_MUI) $(DEPFLAGS_AMIGAOS) $< -o $@

${BINDIR_AMIGAOS}/muitest:	$(OBJS_AMIGAOS) $(SELF_DIR)/src/amigasupport/amigasupport.a
	$(CC_AMIGAOS) -g $(CFLAGS_AMIGAOS) $(LDFLAGS_AMIGAOS) -o $@ $^ $(SELF_DIR)/src/amigasupport/amigasupport.a
	# $(STRIP_AMIGAOS) $@
	cp $@ $(DISTDIR)

$(DEPDIR_AMIGAOS): ; @mkdir -p $@

DEPFILES_AMIGAOS := $(SRCS:%.c=$(DEPDIR_AMIGAOS)/%.d)
$(DEPFILES_AMIGAOS):

include $(wildcard $(DEPFILES_AMIGAOS))

diss: ${BINDIR_AMIGAOS}/muitest
	$(OBJDUMP_AMIGAOS) -S -d -x ${BINDIR_AMIGAOS}/muitest

%.asm: %.c
	$(CC_AMIGAOS) -S $(CFLAGS_AMIGAOS) $(CFLAGS_MUI) $< -o $@

