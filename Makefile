include config.mk

MD2AGUIDE = src/tools/md2amiga/md2aguide.py

.PHONY: clean all tests src examples help directories

all: directories src tests help examples

src:
	cd src ; make all

tests:
	cd tests ; make all

examples:
	cd examples ; make all

help:	README.md
	$(MD2AGUIDE) README.md > README.guide
	cp README.guide $(DISTDIR)
	cd help ; make all

directories:
	mkdir -p target/m68k-amigaos/bin
	mkdir -p target/m68k-amigaos/obj
	mkdir -p target/x86_64-linux/bin
	mkdir -p target/x86_64-linux/obj

clean:
	cd src ; make clean
	cd tests ; make clean
	cd examples ; make clean
	cd help ; make clean
	rm -rf target
	rm -f README.guide
