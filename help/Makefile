include ../config.mk

MD2AGUIDE = ../src/tools/md2amiga/md2aguide.py

.PHONY: clean all

all:	RefAmiga.guide RefCore.guide
	mkdir -p $(DISTDIR)/help
	cp *.guide $(DISTDIR)/help/

%.guide : %.md
	$(MD2AGUIDE) $< > $@

clean:
	rm -f *.guide

