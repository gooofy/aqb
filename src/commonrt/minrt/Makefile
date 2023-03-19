include ../../../config.mk

CFLAGS_AMIGAOS2   = -O3 -march=68000 -mcpu=68000 -nostdlib -fno-builtin
#-mcrt=nix20 -fomit-frame-pointer
# -I$(SELF_DIR)/src/amigasupport

LDFLAGS_AMIGAOS  =

STARTUP = $(SELF_DIR)/src/commonrt/startup/startup.o

SRCS = amigasupport.c break_handler.c  cstartup.c  debugger.c  io.c \
	   strings.c math.c mem.c

OBJS_AMIGAOS := $(patsubst %,${OBJDIR_AMIGAOS}/%.o,$(basename $(SRCS)))

.PHONY: clean all

all:	minrt.a

clean:
	rm -f $(OBJDIR_AMIGAOS)/*.o minrt.a
	rm -rf $(DEPDIR_AMIGAOS)

$(OBJDIR_AMIGAOS)/%.o : %.c $(DEPDIR_AMIGAOS)/%.d | $(DEPDIR_AMIGAOS)
	$(CC_AMIGAOS) -c -g $(CFLAGS_AMIGAOS2) $(DEPFLAGS_AMIGAOS) $< -o $@

minrt.a:	$(OBJS_AMIGAOS)
	$(AR_AMIGAOS) -crs minrt.a $^
	cp minrt.a $(DISTDIR)/lib

$(DEPDIR_AMIGAOS): ; @mkdir -p $@

DEPFILES_AMIGAOS := $(SRCS:%.c=$(DEPDIR_AMIGAOS)/%.d)
$(DEPFILES_AMIGAOS):

include $(wildcard $(DEPFILES_AMIGAOS))

#diss: ${BINDIR_AMIGAOS}/rttest
#	$(OBJDUMP_AMIGAOS) -S -d -x ${BINDIR_AMIGAOS}/rttest

%.asm: %.c
	$(CC_AMIGAOS) -S $(CFLAGS_AMIGAOS2) $< -o $@

