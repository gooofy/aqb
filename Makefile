.PHONY: clean all tests src

all: src tests

src:
	cd src ; make all

tests:
	cd tests ; make all

clean:
	cd src ; make clean
	cd tests ; make clean
