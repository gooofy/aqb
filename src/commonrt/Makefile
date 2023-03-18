include ../../config.mk

.PHONY: clean minrt startup

all: minrt startup

minrt:
	cd minrt ; make all

startup:
	cd startup ; make all

clean:
	rm -f *.sym *.o *.a
	cd minrt ; make clean
	cd startup ; make clean
