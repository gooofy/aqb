.PHONY: clean _brt _aqb

all: _brt _aqb

_brt:
	cd _brt ; make all

_aqb:
	cd _aqb ; make all

clean:
	rm -f *.sym
	cd _brt ; make clean
	cd _aqb ; make clean
