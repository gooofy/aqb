include ../../../config.mk

.PHONY: clean all

all: OSUtility.sym

OSUtility.sym : OSUtility.bas
	$(AQB) -L .. -I -d _aqb -s OSUtility.sym $<
	# pushd .. && rm -f OSUtility.sym OSUtility.a && ln -s OSUtility/OSUtility.sym && ln -s OSUtility/OSUtility.a && popd
	pushd .. && rm -f OSUtility.sym && ln -s OSUtility/OSUtility.sym && popd
	cp $< $(DISTDIR)/libsrc
	cp $@ $(DISTDIR)/lib
	rm -f $(DISTDIR)/lib/*.uaem

clean:
	rm -f *.sym aqb.log

