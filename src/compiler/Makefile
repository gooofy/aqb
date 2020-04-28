
SRCS = 	aqb.c scanner.c hashmap.c util.c parser.c \
		errormsg.c absyn.c symbol.c prabsyn.c semant.c translate.c \
		frame.c tree.c temp.c assem.c printtree.c env.c types.c \
		canon.c codegen.c regalloc.c graph.c liveness.c color.c flowgraph.c table.c

OBJS := $(patsubst %,%.o,$(basename $(SRCS)))

DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

CFLAGS = -Wall -Werror -g
LDFLAGS = -lm

CC = gcc

.PHONY: clean all
all:	aqb
clean:
	rm -f *.o aqb

%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(DEPFLAGS) $< -o $@

aqb:	$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(DEPDIR): ; @mkdir -p $@

DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)
$(DEPFILES):

include $(wildcard $(DEPFILES))

