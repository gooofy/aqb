include ../../config.mk

SRCS = 	acs.c scanner.c parser.c semantics.c hashmap.c util.c options.c \
		errormsg.c symbol.c ir.c codegen.c assem.c \
		temp.c compiler.c logger.c linscan.c \
		table.c

OBJS_AMIGAOS := $(patsubst %,${OBJDIR_AMIGAOS}/%.o,$(basename $(SRCS)))
OBJS_LINUX   := $(patsubst %,${OBJDIR_LINUX}/%.o,$(basename $(SRCS)))

.PHONY: clean all
all:	$(BINDIR_AMIGAOS)/acs $(BINDIR_LINUX)/acs

clean:
	rm -f $(OBJDIR_AMIGAOS)/*.o $(BINDIR_AMIGAOS)/acs $(OBJDIR_LINUX)/*.o $(BINDIR_LINUX)/acs
	rm -rf $(DEPDIR_AMIGAOS) $(DEPDIR_LINUX)

$(OBJDIR_AMIGAOS)/%.o : %.c $(DEPDIR_AMIGAOS)/%.d | $(DEPDIR_AMIGAOS)
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) $(DEPFLAGS_AMIGAOS) $< -o $@

$(OBJDIR_LINUX)/%.o : %.c $(DEPDIR_LINUX)/%.d | $(DEPDIR_LINUX)
	$(CC_LINUX) -c $(CFLAGS_LINUX) $(DEPFLAGS_LINUX) $< -o $@

${BINDIR_AMIGAOS}/acs:	$(OBJS_AMIGAOS) $(SELF_DIR)/src/amigasupport/amigasupport.a
	$(CC_AMIGAOS) $(CFLAGS_AMIGAOS) -o $@ $^ $(LDFLAGS_AMIGAOS) $(SELF_DIR)/src/amigasupport/amigasupport.a
	$(STRIP_AMIGAOS) $@
	cp $@ $(DISTDIR)
	rm -f $(DISTDIR)/*.uaem

${BINDIR_LINUX}/acs:	$(OBJS_LINUX)
	$(CC_LINUX) $(CFLAGS_LINUX) -o $@ $^ $(LDFLAGS_LINUX)

$(DEPDIR_AMIGAOS): ; @mkdir -p $@
$(DEPDIR_LINUX): ; @mkdir -p $@

DEPFILES_AMIGAOS := $(SRCS:%.c=$(DEPDIR_AMIGAOS)/%.d)
$(DEPFILES_AMIGAOS):

DEPFILES_LINUX := $(SRCS:%.c=$(DEPDIR_LINUX)/%.d)
$(DEPFILES_LINUX):

include $(wildcard $(DEPFILES_AMIGAOS))
include $(wildcard $(DEPFILES_LINUX))

%.asm: %.c
	$(CC_AMIGAOS) -S $(CFLAGS_AMIGAOS) $< -o $@
