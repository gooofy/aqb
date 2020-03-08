
CFLAGS = -Wall -g 
LDFLAGS = -lm

.PHONY: clean all
all:	aqb
clean: 
	rm -f *.o aqb

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

aqb:	aqb.o scanner.o hashmap.o util.o parser.o \
		errormsg.o absyn.o symbol.o prabsyn.o semant.o translate.o \
		m68kframe.o tree.o temp.o assem.o printtree.o env.o types.o \
		canon.o codegen.o regalloc.o graph.o liveness.o flowgraph.o color.o table.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

#		errormsg.o absyn.o symbol.o table.o \
#	    prabsyn.o types.o semant.o env.o temp.o tree.o m68kframe.o translate.o \
#	    assem.o canon.o codegen.o color.o escape.o flowgraph.o graph.o liveness.o \
#	    regalloc.o printtree.o

