include ../../../config.mk

.PHONY: clean all

all: OSGraphics.sym

OSGraphics.sym : OSGraphics.bas
	$(AQB) -L .. -I -d _brt -s OSGraphics.sym $<
	# pushd .. && rm -f OSGraphics.sym OSGraphics.a && ln -s OSGraphics/OSGraphics.sym && ln -s OSGraphics/OSGraphics.a && popd
	pushd .. && rm -f OSGraphics.sym && ln -s OSGraphics/OSGraphics.sym && popd
	cp $< $(DISTDIR)/libsrc
	cp $@ $(DISTDIR)/lib
	rm -f $(DISTDIR)/lib/*.uaem

clean:
	rm -f *.sym aqb.log

