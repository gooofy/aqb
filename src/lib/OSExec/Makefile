include ../../../config.mk

.PHONY: clean all

all: OSExec.sym

OSExec.sym : OSExec.bas
	$(AQB) -L .. -I -d _aqb -s OSExec.sym $<
	# pushd .. && rm -f OSExec.sym OSExec.a && ln -s OSExec/OSExec.sym && ln -s OSExec/OSExec.a && popd
	pushd .. && rm -f OSExec.sym && ln -s OSExec/OSExec.sym && popd
	cp $< $(DISTDIR)/libsrc
	cp $@ $(DISTDIR)/lib
	rm -f $(DISTDIR)/lib/*.uaem

clean:
	rm -f *.sym aqb.log

