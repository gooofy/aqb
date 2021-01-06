.PHONY: clean _brt _aqb minbrt

all: _brt _aqb minbrt

_brt:
	cd _brt ; make all

_aqb:
	cd _aqb ; make all

minbrt:
	cd minbrt ; make all

clean:
	rm -f *.sym
	cd _brt ; make clean
	cd _aqb ; make clean
	cd minbrt ; make clean
