include ../../config.mk

TESTNAME = subtest11
TESTDIR = $(EMUDIR)/aqb_tests/$(TESTNAME)
TESTDIR_OPT = $(EMUDIR)/aqb_tests_opt/$(TESTNAME)

AQBFLAGS     = -L ../../src/lib -L $(TESTDIR)
AQBFLAGS_OPT = -O -L ../../src/lib -L $(TESTDIR_OPT)

.PHONY:	clean all dirs

all:	dirs $(TESTDIR)/$(TESTNAME) $(TESTDIR_OPT)/$(TESTNAME)

dirs:
	mkdir -p $(TESTDIR)
	mkdir -p $(TESTDIR_OPT)

clean:
	rm -rf $(TESTDIR)
	rm -rf $(TESTDIR_OPT)

$(TESTDIR)/$(TESTNAME).o:	$(TESTDIR)/$(TESTNAME).s
	$(AS_AMIGAOS) $< -o $@

$(TESTDIR)/$(TESTNAME).s:	$(TESTNAME).bas
	$(AQB) $(AQBFLAGS) -a $@ $(TESTNAME).bas

$(TESTDIR)/$(TESTNAME):	$(TESTDIR)/$(TESTNAME).o
	$(LD_AMIGAOS) ../../src/lib/_brt/startup.o $(TESTDIR)/$(TESTNAME).o ../../src/lib/_brt/_brt.a ../../src/lib/_aqb/_aqb.a -o $(TESTDIR)/$(TESTNAME)

$(TESTDIR_OPT)/$(TESTNAME).o:	$(TESTDIR_OPT)/$(TESTNAME).s
	$(AS_AMIGAOS) $< -o $@

$(TESTDIR_OPT)/$(TESTNAME).s:	$(TESTNAME).bas
	$(AQB) $(AQBFLAGS) -a $@ $(TESTNAME).bas

$(TESTDIR_OPT)/$(TESTNAME):	$(TESTDIR_OPT)/$(TESTNAME).o
	$(LD_AMIGAOS) ../../src/lib/_brt/startup.o $(TESTDIR_OPT)/$(TESTNAME).o ../../src/lib/_brt/_brt.a ../../src/lib/_aqb/_aqb.a -o $(TESTDIR_OPT)/$(TESTNAME)

