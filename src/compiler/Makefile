
OBJDIR_AMIGAOS = ../../target/m68k-amigaos/obj
BINDIR_AMIGAOS = ../../target/m68k-amigaos/bin
EMUDIR_AMIGAOS = /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/

OBJDIR_LINUX = ../../target/x86_64-linux/obj
BINDIR_LINUX = ../../target/x86_64-linux/bin

SRCS = 	aqb.c scanner.c hashmap.c util.c frontend.c options.c \
		errormsg.c symbol.c translate.c linscan.c \
		frame.c tree.c temp.c assem.c printtree.c env.c types.c \
		canon.c codegen.c regalloc.c liveness.c color.c flowgraph.c table.c

OBJS_AMIGAOS := $(patsubst %,${OBJDIR_AMIGAOS}/%.o,$(basename $(SRCS)))
OBJS_LINUX   := $(patsubst %,${OBJDIR_LINUX}/%.o,$(basename $(SRCS)))

DEPDIR_AMIGAOS := ../../target/m68k-amigaos/.deps
DEPDIR_LINUX   := ../../target/x86_64-linux/.deps
DEPFLAGS_AMIGAOS = -MT $@ -MMD -MP -MF $(DEPDIR_AMIGAOS)/$*.d
DEPFLAGS         = -MT $@ -MMD -MP -MF $(DEPDIR_LINUX)/$*.d

CFLAGS = -Wall -Werror -g
# CFLAGS = -Wall -Werror -g -pg
LDFLAGS = -lm

CC = gcc

CFLAGS_AMIGAOS = -O3 -Wall -Werror -march=68000 -mcpu=68000
ASFLAGS_AMIGAOS = -march=68000 -mcpu=68000
LDFLAGS_AMIGAOS = -lm

CC_AMIGAOS = m68k-amigaos-gcc
AS_AMIGAOS = m68k-amigaos-as
LD_AMIGAOS = m68k-amigaos-ld
AR_AMIGAOS = m68k-amigaos-ar

.PHONY: clean all
all:	$(BINDIR_AMIGAOS)/aqb $(BINDIR_LINUX)/aqb

clean:
	rm -f $(OBJDIR_AMIGAOS)/*.o $(BINDIR_AMIGAOS)/aqb $(OBJDIR_LINUX)/*.o $(BINDIR_LINUX)/aqb
	rm -rf $(DEPDIR_AMIGAIS) $(DEPDIR_LINUX)

$(OBJDIR_AMIGAOS)/%.o : %.c $(DEPDIR_AMIGAOS)/%.d | $(DEPDIR_AMIGAOS)
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) $(DEPFLAGS_AMIGAOS) $< -o $@

$(OBJDIR_LINUX)/%.o : %.c $(DEPDIR_LINUX)/%.d | $(DEPDIR_LINUX)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) $< -o $@

${BINDIR_AMIGAOS}/aqb:	$(OBJS_AMIGAOS)
	$(CC_AMIGAOS) $(CFLAGS_AMIGAOS) $(LDFLAGS_AMIGAOS) -o $@ $^
	cp $@ $(EMUDIR_AMIGAOS)

${BINDIR_LINUX}/aqb:	$(OBJS_LINUX)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(DEPDIR_AMIGAOS): ; @mkdir -p $@
$(DEPDIR_LINUX): ; @mkdir -p $@

DEPFILES_AMIGAOS := $(SRCS:%.c=$(DEPDIR_AMIGAOS)/%.d)
$(DEPFILES_AMIGAOS):

DEPFILES_LINUX := $(SRCS:%.c=$(DEPDIR_LINUX)/%.d)
$(DEPFILES_LINUX):

include $(wildcard $(DEPFILES_AMIGAOS))
include $(wildcard $(DEPFILES_LINUX))

