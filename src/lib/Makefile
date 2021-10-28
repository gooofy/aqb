.PHONY: clean _brt _aqb minbrt IFFSupport AnimSupport

all: _brt _aqb minbrt IFFSupport AnimSupport

_brt:
	cd _brt ; make all

_aqb:
	cd _aqb ; make all

minbrt:
	cd minbrt ; make all

IFFSupport:
	cd IFFSupport ; make all

AnimSupport:
	cd AnimSupport ; make all

clean:
	rm -f *.sym *.o *.a
	cd _brt ; make clean
	cd _aqb ; make clean
	cd minbrt ; make clean
	cd IFFSupport ; make clean
	cd AnimSupport ; make clean
