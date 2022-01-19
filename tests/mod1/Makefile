include ../../config.mk

TESTNAME = mod1
TESTDIR = $(EMUDIR)/aqb_tests/$(TESTNAME)
TESTDIR_OPT = $(EMUDIR)/aqb_tests_opt/$(TESTNAME)

AQBFLAGS     = -L ../../src/lib -L $(TESTDIR)
AQBFLAGS_OPT = -O -L ../../src/lib -L $(TESTDIR_OPT)

.PHONY:	clean all dirs

all:	dirs $(TESTDIR)/mod1 $(TESTDIR_OPT)/mod1

dirs:
	mkdir -p $(TESTDIR)
	mkdir -p $(TESTDIR_OPT)

clean:
	rm -rf $(TESTDIR)
	rm -rf $(TESTDIR_OPT)

$(TESTDIR)/foo.sym:	foo.bas
	$(AQB) $(AQBFLAGS) -a $(TESTDIR)/foo.s -s $(TESTDIR)/foo.sym foo.bas

$(TESTDIR)/bar.sym:	bar.bas
	$(AQB) $(AQBFLAGS) -a $(TESTDIR)/bar.s -s $(TESTDIR)/bar.sym bar.bas

$(TESTDIR_OPT)/foo.sym:	foo.bas
	$(AQB) $(AQBFLAGS_OPT) -a $(TESTDIR_OPT)/foo.s -s $(TESTDIR_OPT)/foo.sym foo.bas

$(TESTDIR_OPT)/bar.sym:	bar.bas
	$(AQB) $(AQBFLAGS_OPT) -a $(TESTDIR_OPT)/bar.s -s $(TESTDIR_OPT)/bar.sym bar.bas

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


$(TESTDIR_OPT)/foo.o $(TESTDIR_OPT)/bar.o $(TESTDIR_OPT)/mod1.o : $(TESTDIR_OPT)/%.o:	$(TESTDIR_OPT)/%.s
	$(AS_AMIGAOS) $< -o $@

$(TESTDIR_OPT)/mod1.s:	mod1.bas $(TESTDIR_OPT)/foo.sym $(TESTDIR_OPT)/bar.sym
	$(AQB) $(AQBFLAGS_OPT) -a $(TESTDIR_OPT)/mod1.s mod1.bas

$(TESTDIR_OPT)/mod1:	$(TESTDIR_OPT)/mod1.o $(TESTDIR_OPT)/foo.o $(TESTDIR_OPT)/bar.o
	$(LD_AMIGAOS) ../../src/lib/_brt/startup.o $(TESTDIR_OPT)/mod1.o $(TESTDIR_OPT)/foo.o $(TESTDIR_OPT)/bar.o ../../src/lib/_aqb/_aqb.a ../../src/lib/_brt/_brt.a -o $(TESTDIR_OPT)/mod1

