include ../../../config.mk

SRCS = 	areaptrn.c

OBJS_AMIGAOS := $(patsubst %,${OBJDIR_AMIGAOS}/%.o,$(basename $(SRCS)))

.PHONY: clean all
all:	$(BINDIR_AMIGAOS)/areaptrn

clean:
	rm -f $(OBJDIR_AMIGAOS)/*.o $(BINDIR_AMIGAOS)/areaptrn
	rm -rf $(DEPDIR_AMIGAOS)

$(OBJDIR_AMIGAOS)/%.o : %.c $(DEPDIR_AMIGAOS)/%.d | $(DEPDIR_AMIGAOS)
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) $(DEPFLAGS_AMIGAOS) $< -o $@

${BINDIR_AMIGAOS}/areaptrn:	$(OBJS_AMIGAOS)
	$(CC_AMIGAOS) $(CFLAGS_AMIGAOS) $(LDFLAGS_AMIGAOS) -o $@ $^
	# $(STRIP_AMIGAOS) $@
	cp $@ $(DISTDIR)

$(DEPDIR_AMIGAOS): ; @mkdir -p $@

DEPFILES_AMIGAOS := $(SRCS:%.c=$(DEPDIR_AMIGAOS)/%.d)
$(DEPFILES_AMIGAOS):

include $(wildcard $(DEPFILES_AMIGAOS))

diss: ${BINDIR_AMIGAOS}/areaptrn
	$(OBJDUMP_AMIGAOS) -S -d -x ${BINDIR_AMIGAOS}/areaptrn

