include ../../../config.mk

.PHONY: clean all

all: OSIntuition.sym

OSIntuition.sym : OSIntuition.bas
	$(AQB) -L .. -I -d _aqb -s OSIntuition.sym $<
	# pushd .. && rm -f OSIntuition.sym OSIntuition.a && ln -s OSIntuition/OSIntuition.sym && ln -s OSIntuition/OSIntuition.a && popd
	pushd .. && rm -f OSIntuition.sym && ln -s OSIntuition/OSIntuition.sym && popd
	cp $< $(DISTDIR)/libsrc
	cp $@ $(DISTDIR)/lib
	rm -f $(DISTDIR)/lib/*.uaem

clean:
	rm -f *.sym aqb.log

