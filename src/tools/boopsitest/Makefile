include ../../../config.mk

CFLAGS_AMIGAOS2   = -O3 -march=68000 -mcpu=68000 -mcrt=nix20 -I$(SELF_DIR)/src/amigasupport

SRCS = 	bitest.c

OBJS_AMIGAOS := $(patsubst %,${OBJDIR_AMIGAOS}/%.o,$(basename $(SRCS)))

.PHONY: clean all
all:	$(BINDIR_AMIGAOS)/bitest

clean:
	rm -f $(OBJDIR_AMIGAOS)/*.o $(BINDIR_AMIGAOS)/bitest bitest.asm
	rm -rf $(DEPDIR_AMIGAOS)

$(OBJDIR_AMIGAOS)/%.o : %.c $(DEPDIR_AMIGAOS)/%.d | $(DEPDIR_AMIGAOS)
	$(CC_AMIGAOS) -c -g $(CFLAGS_AMIGAOS) $(DEPFLAGS_AMIGAOS) $< -o $@

${BINDIR_AMIGAOS}/bitest:	$(OBJS_AMIGAOS) $(SELF_DIR)/src/amigasupport/amigasupport.a
	$(CC_AMIGAOS) -g $(CFLAGS_AMIGAOS) $(LDFLAGS_AMIGAOS) -o $@ $^ $(SELF_DIR)/src/amigasupport/amigasupport.a
	# $(STRIP_AMIGAOS) $@
	cp $@ $(DISTDIR)

$(DEPDIR_AMIGAOS): ; @mkdir -p $@

DEPFILES_AMIGAOS := $(SRCS:%.c=$(DEPDIR_AMIGAOS)/%.d)
$(DEPFILES_AMIGAOS):

include $(wildcard $(DEPFILES_AMIGAOS))

diss: ${BINDIR_AMIGAOS}/bitest
	$(OBJDUMP_AMIGAOS) -S -d -x ${BINDIR_AMIGAOS}/bitest

%.asm: %.c
	$(CC_AMIGAOS) -S $(CFLAGS_AMIGAOS) $< -o $@

