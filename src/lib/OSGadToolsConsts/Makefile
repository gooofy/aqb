include ../../../config.mk

.PHONY: clean all

all: OSGadToolsConsts.sym

OSGadToolsConsts.sym : OSGadToolsConsts.bas
	$(AQB) -L .. -I -d _aqb -s OSGadToolsConsts.sym $<
	pushd .. && rm -f OSGadToolsConsts.sym OSGadToolsConsts.a && ln -s OSGadToolsConsts/OSGadToolsConsts.sym && popd
	cp $< $(DISTDIR)/libsrc
	cp $@ $(DISTDIR)/lib
	rm -f $(DISTDIR)/lib/*.uaem

clean:
	rm -f *.sym aqb.log

