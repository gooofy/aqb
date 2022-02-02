include ../../../config.mk

.PHONY: clean all

all: OSDevices.sym

OSDevices.sym : OSDevices.bas
	$(AQB) -L .. -I -d _aqb -s OSDevices.sym $<
	# pushd .. && rm -f OSDevices.sym OSDevices.a && ln -s OSDevices/OSDevices.sym && ln -s OSDevices/OSDevices.a && popd
	pushd .. && rm -f OSDevices.sym && ln -s OSDevices/OSDevices.sym && popd
	cp $< $(DISTDIR)/libsrc
	cp $@ $(DISTDIR)/lib
	rm -f $(DISTDIR)/lib/*.uaem

clean:
	rm -f *.sym aqb.log

