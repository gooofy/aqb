.PHONY: clean _brt _aqb minbrt IFFSupport

all: _brt _aqb minbrt IFFSupport

_brt:
	cd _brt ; make all

_aqb:
	cd _aqb ; make all

minbrt:
	cd minbrt ; make all

IFFSupport:
	cd IFFSupport ; make all

clean:
	rm -f *.sym *.o *.a
	cd _brt ; make clean
	cd _aqb ; make clean
	cd minbrt ; make clean
	cd IFFSupport ; make clean
