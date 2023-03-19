include ../../../config.mk

CFLAGS_AMIGAOS2   = -O3 -march=68000 -mcpu=68000 -nostdlib -I$(SELF_DIR)/src/commonrt/minrt
#-mcrt=nix20 -fomit-frame-pointer
# -I$(SELF_DIR)/src/amigasupport

LDFLAGS_AMIGAOS  =

STARTUP = $(SELF_DIR)/src/commonrt/startup/startup.o
MINRT   = $(SELF_DIR)/src/commonrt/minrt/minrt.a

SRCS = rttest.c

OBJS_AMIGAOS := $(patsubst %,${OBJDIR_AMIGAOS}/%.o,$(basename $(SRCS)))

.PHONY: clean all
all:	$(BINDIR_AMIGAOS)/rttest

clean:
	rm -f $(OBJDIR_AMIGAOS)/*.o $(BINDIR_AMIGAOS)/rttest rttest.asm
	rm -rf $(DEPDIR_AMIGAOS)

$(OBJDIR_AMIGAOS)/%.o : %.c $(DEPDIR_AMIGAOS)/%.d | $(DEPDIR_AMIGAOS)
	$(CC_AMIGAOS) -c -g $(CFLAGS_AMIGAOS2) $(DEPFLAGS_AMIGAOS) $< -o $@

${BINDIR_AMIGAOS}/rttest:	$(OBJS_AMIGAOS)
	$(CC_AMIGAOS) -g $(CFLAGS_AMIGAOS2) $(LDFLAGS_AMIGAOS) -o $@ $(STARTUP) $^ $(MINRT)
	# $(STRIP_AMIGAOS) $@
	cp $@ $(DISTDIR)

$(DEPDIR_AMIGAOS): ; @mkdir -p $@

DEPFILES_AMIGAOS := $(SRCS:%.c=$(DEPDIR_AMIGAOS)/%.d)
$(DEPFILES_AMIGAOS):

include $(wildcard $(DEPFILES_AMIGAOS))

diss: ${BINDIR_AMIGAOS}/rttest
	$(OBJDUMP_AMIGAOS) -S -d -x ${BINDIR_AMIGAOS}/rttest

%.asm: %.c
	$(CC_AMIGAOS) -S $(CFLAGS_AMIGAOS2) $< -o $@

	#$(CC_AMIGAOS) -g $(CFLAGS_AMIGAOS2) $(LDFLAGS_AMIGAOS) -o $@ $^ $(SELF_DIR)/src/amigasupport/amigasupport.a
