include ../../../config.mk

SRCS = 	printfloat.c

OBJS_AMIGAOS := $(patsubst %,${OBJDIR_AMIGAOS}/%.o,$(basename $(SRCS)))

.PHONY: clean all
all:	$(BINDIR_AMIGAOS)/printfloat

clean:
	rm -f $(OBJDIR_AMIGAOS)/*.o $(BINDIR_AMIGAOS)/printfloat
	rm -rf $(DEPDIR_AMIGAOS)

$(OBJDIR_AMIGAOS)/%.o : %.c $(DEPDIR_AMIGAOS)/%.d | $(DEPDIR_AMIGAOS)
	$(CC_AMIGAOS) -v -c $(CFLAGS_AMIGAOS) $(DEPFLAGS_AMIGAOS) $< -o $@

${BINDIR_AMIGAOS}/printfloat:	$(OBJS_AMIGAOS)
	$(CC_AMIGAOS) $(CFLAGS_AMIGAOS) $(LDFLAGS_AMIGAOS) -v -o $@ $^
	# $(STRIP_AMIGAOS) $@
	cp $@ $(DISTDIR)

$(DEPDIR_AMIGAOS): ; @mkdir -p $@

DEPFILES_AMIGAOS := $(SRCS:%.c=$(DEPDIR_AMIGAOS)/%.d)
$(DEPFILES_AMIGAOS):

include $(wildcard $(DEPFILES_AMIGAOS))

diss: ${BINDIR_AMIGAOS}/printfloat
	$(OBJDUMP_AMIGAOS) -S -d -x ${BINDIR_AMIGAOS}/printfloat

