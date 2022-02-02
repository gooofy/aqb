include ../../../config.mk

.PHONY: clean all

all: OSHardware.sym

OSHardware.sym : OSHardware.bas
	$(AQB) -L .. -I -d _aqb -s OSHardware.sym $<
	# pushd .. && rm -f OSHardware.sym OSHardware.a && ln -s OSHardware/OSHardware.sym && ln -s OSHardware/OSHardware.a && popd
	pushd .. && rm -f OSHardware.sym && ln -s OSHardware/OSHardware.sym && popd
	cp $< $(DISTDIR)/libsrc
	cp $@ $(DISTDIR)/lib
	rm -f $(DISTDIR)/lib/*.uaem

clean:
	rm -f *.sym aqb.log

