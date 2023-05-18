include ../../config.mk

TESTNAME = mod1
TESTDIR = $(EMUDIR)/aqb_tests/$(TESTNAME)

AQBFLAGS     = -L ../../src/lib -L $(TESTDIR)

.PHONY:	clean all dirs

all:	dirs $(TESTDIR)/mod1

dirs:
	mkdir -p $(TESTDIR)

clean:
	rm -rf $(TESTDIR)

$(TESTDIR)/foo.sym:	foo.bas
	$(AQB) $(AQBFLAGS) -a $(TESTDIR)/foo.s -s $(TESTDIR)/foo.sym foo.bas

$(TESTDIR)/bar.sym:	bar.bas
	$(AQB) $(AQBFLAGS) -a $(TESTDIR)/bar.s -s $(TESTDIR)/bar.sym bar.bas

# foo.s:		foo.sym
# 	noop
#
# bar.sym:	bar.bas
# 	$(AQB) $(AQBFLAGS) -s bar.bas
#
# bar.s:		bar.sym
# 	noop

$(TESTDIR)/foo.o $(TESTDIR)/bar.o $(TESTDIR)/mod1.o : $(TESTDIR)/%.o:	$(TESTDIR)/%.s
	$(AS_AMIGAOS) $< -o $@

$(TESTDIR)/mod1.s:	mod1.bas $(TESTDIR)/foo.sym $(TESTDIR)/bar.sym
	$(AQB) $(AQBFLAGS) -a $(TESTDIR)/mod1.s mod1.bas

$(TESTDIR)/mod1:	$(TESTDIR)/mod1.o $(TESTDIR)/foo.o $(TESTDIR)/bar.o
	$(LD_AMIGAOS) ../../src/lib/_brt/startup.o $(TESTDIR)/mod1.o $(TESTDIR)/foo.o $(TESTDIR)/bar.o ../../src/lib/_aqb/_aqb.a ../../src/lib/_brt/_brt.a -o $(TESTDIR)/mod1


