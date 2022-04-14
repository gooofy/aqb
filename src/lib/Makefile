include ../../config.mk

.PHONY: clean _brt minbrt IFFSupport AnimSupport OSExec OSUtility OSHardware OSGraphics OSDevices OSIntuition OSGadTools _aqb GadToolsSupport

all: _brt minbrt OSExec OSUtility OSHardware OSGraphics OSDevices OSIntuition OSGadTools _aqb IFFSupport AnimSupport GadToolsSupport

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

OSExec:
	cd OSExec ; make all

OSUtility:
	cd OSUtility ; make all

OSHardware:
	cd OSHardware ; make all

OSGraphics:
	cd OSGraphics ; make all

OSDevices:
	cd OSDevices ; make all

OSIntuition:
	cd OSIntuition ; make all

OSGadTools:
	cd OSGadTools ; make all

GadToolsSupport:
	cd GadToolsSupport ; make all

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
	cd OSExec ; make clean
	cd OSHardware ; make clean
	cd OSGraphics ; make clean
	cd OSDevices ; make clean
	cd OSIntuition ; make clean
	cd OSUtility ; make clean
	cd OSGadTools ; make clean
	cd GadToolsSupport ; make clean
