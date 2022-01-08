include ../../config.mk

.PHONY: clean _brt _aqb minbrt IFFSupport AnimSupport OSGadToolsConsts UISupport

all: _brt _aqb minbrt IFFSupport AnimSupport OSGadToolsConsts UISupport

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

OSGadToolsConsts:
	cd OSGadToolsConsts ; make all

UISupport:
	cd UISupport ; make all

clean:
	rm -f *.sym *.o *.a
	rm -f $(DISTDIR)/lib/*.sym
	rm -f $(DISTDIR)/lib/*.a
	rm -f $(DISTDIR)/lib/*.o
	cd _brt ; make clean
	cd _aqb ; make clean
	cd minbrt ; make clean
	cd IFFSupport ; make clean
	cd AnimSupport ; make clean
	cd OSGadToolsConsts ; make clean
	cd UISupport ; make clean
