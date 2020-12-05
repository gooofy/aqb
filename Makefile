.PHONY: clean all tests src examples directories

all: directories src tests examples

src:
	cd src ; make all

tests:
	cd tests ; make all

examples:
	cd examples ; make all

directories:
	mkdir -p target/m68k-amigaos/bin
	mkdir -p target/m68k-amigaos/obj
	mkdir -p target/x86_64-linux/bin
	mkdir -p target/x86_64-linux/obj

clean:
	cd src ; make clean
	cd tests ; make clean
	cd examples ; make clean
	rm -rf target
