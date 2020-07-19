.PHONY: clean all tests src examples

all: src tests examples

src:
	cd src ; make all

tests:
	cd tests ; make all

examples:
	cd examples ; make all

clean:
	cd src ; make clean
	cd tests ; make clean
	cd examples ; make clean
