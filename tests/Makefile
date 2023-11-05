include ../config.mk

ACSLIBS=../src/lib
ACSFLAGS     = -L $(ACSLIBS) -l _urt

SRCS = overload1.cs

#SRCS = cond1.bas cond2.bas cond3.bas cond4.bas cond5.bas cond6.bas \
#		const1.bas const2.bas const3.bas constprop.bas \
#		ctrl1.bas ctrl2.bas ctrl3.bas ctrl4.bas ctrl5.bas ctrl6.bas ctrl7.bas ctrl8.bas ctrl9.bas \
#		dynmem1.bas \
#		exp1.bas exp2.bas exp3.bas \
#		extcall2.bas \
#		loop1.bas loop2.bas loop3.bas loop4.bas loop5.bas loop6.bas loop7.bas loop8.bas \
#		loop9.bas loop10.bas loop11.bas loop12.bas loop13.bas loop14.bas loop15.bas \
#	    opbool1.bas opbool2.bas \
#		opbyte.bas opint.bas oplong.bas \
#		opsingle1.bas opsingle2.bas \
#		opubyte.bas opuint.bas opulong.bas \
#	    oop1.bas oop2.bas oop3.bas oop4.bas oop5.bas oop6.bas oop7.bas oop8.bas oop9.bas \
#		oop10.bas oop11.bas oop12.bas oop13.bas oop14.bas oop15.bas oop16.bas oop17.bas oop18.bas \
#	    coll1.bas coll2.bas coll3.bas coll4.bas coll5.bas \
#	    scopes1.bas scopes2.bas scopes3.bas \
# 	    string1.bas string2.bas string3.bas string4.bas string5.bas string6.bas \
# 	    subtest1.bas subtest2.bas subtest3.bas subtest4.bas subtest5.bas \
# 	    subtest6.bas subtest7.bas subtest8.bas subtest9.bas \
#	    subtest10.bas subtest12.bas subtest13.bas subtest14.bas subtest15.bas subtest16.bas \
# 	    subtest17.bas subtest18.bas subtest19.bas subtest20.bas subtest21.bas subtest22.bas \
#	    subtest23.bas subtest24.bas subtest25.bas subtest26.bas subtest27.bas \
#	    types1.bas types2.bas types3.bas types4.bas types5.bas types6.bas types7.bas \
#	    var1.bas var2.bas swap.bas \
#	    sarray1.bas sarray2.bas sarray3.bas sarray4.bas sarray5.bas sarray6.bas \
#	    carray1.bas carray2.bas carray3.bas carray4.bas carray5.bas carray6.bas carray7.bas \
#		carray8.bas carray9.bas carray10.bas carray11.bas \
#	    io1.bas io2.bas io3.bas io4.bas io5.bas io6.bas io7.bas

ASMDIR_AMIGAOS_TESTS = acs_tests/asm
OBJDIR_AMIGAOS_TESTS = acs_tests/obj
BINDIR_AMIGAOS_TESTS = acs_tests/bin

#ASMS = $(patsubst %,${EMUDIR}/${ASMDIR_AMIGAOS_TESTS}/%.s,$(basename $(SRCS)))
#OBJS = $(patsubst %,${EMUDIR}/${OBJDIR_AMIGAOS_TESTS}/%.o,$(basename $(SRCS)))
BINS = $(patsubst %,${EMUDIR}/${BINDIR_AMIGAOS_TESTS}/%,$(basename $(SRCS)))

.PHONY: clean all mod1 subtest11 dirs runtests
.PRECIOUS: $(EMUDIR)/$(ASMDIR_AMIGAOS_TESTS)/%.s

all: dirs $(BINS)
# all: dirs $(BINS) mod1 subtest11 runtests

$(BINS) : $(EMUDIR)/$(BINDIR_AMIGAOS_TESTS)/% : %.cs
	$(ACS) $(ACSFLAGS) -v -A $(EMUDIR)/${ASMDIR_AMIGAOS_TESTS}/$<.s -o $@ $(basename $<) $<

#$(ASMS) : $(EMUDIR)/$(ASMDIR_AMIGAOS_TESTS)/%.s : %.bas
#	$(AQB) $(AQBFLAGS) -a $@ $<
#
#$(OBJS) : $(EMUDIR)/$(OBJDIR_AMIGAOS_TESTS)/%.o : $(EMUDIR)/$(ASMDIR_AMIGAOS_TESTS)/%.s
#	$(AS_AMIGAOS) $(ASFLAGS_AMIGAOS) $< -o $@
#
#$(BINS) : $(EMUDIR)/$(BINDIR_AMIGAOS_TESTS)/% : $(EMUDIR)/$(OBJDIR_AMIGAOS_TESTS)/%.o
#	$(LD_AMIGAOS) ../src/lib/_brt/startup.o $< ../src/lib/_brt/_brt.a -o $@
#	$(STRIP_AMIGAOS) $@

mod1:
	cd mod1 ; make all
subtest11:
	cd subtest11 ; make all

runtests:
	echo "; AQB test suite" > $(EMUDIR)/runtests
	for t in $(BINS); do BN=`basename $$t` ; echo "echo $$BN" >> $(EMUDIR)/runtests ; echo "$(BINDIR_AMIGAOS_TESTS)/$$BN" >> $(EMUDIR)/runtests ; done
	# BN="mod1" ; echo "echo $$BN" >> $(EMUDIR)/runtests ; echo "aqb_tests/$$BN/$$BN" >> $(EMUDIR)/runtests
	# BN="subtest11" ; echo "echo $$BN" >> $(EMUDIR)/runtests ; echo "aqb_tests/$$BN/$$BN" >> $(EMUDIR)/runtests
	for t in $(BINS_OPT); do BN=`basename $$t` ; echo "echo $$BN optimized" >> $(EMUDIR)/runtests ; echo "$(BINDIR_AMIGAOS_TESTS_OPT)/$$BN" >> $(EMUDIR)/runtests ; done
	# BN="mod1" ; echo "echo $$BN optimized" >> $(EMUDIR)/runtests ; echo "aqb_tests/$$BN/$$BN" >> $(EMUDIR)/runtests
	# BN="subtest11" ; echo "echo $$BN optimized" >> $(EMUDIR)/runtests ; echo "aqb_tests/$$BN/$$BN" >> $(EMUDIR)/runtests
	# cat $(EMUDIR)/runtests

test: all
	for t in $(BINS); do BN=`basename $$t` ; echo "$(LXA) -r $(LXA_ROM) \"$(WORKDIR)/$(BINDIR_AMIGAOS_TESTS)/$$BN\"" ; $(LXA) -r $(LXA_ROM) "$(WORKDIR)/$(BINDIR_AMIGAOS_TESTS)/$$BN" || break ; done

single:
	$(LXA) -r $(LXA_ROM) "$(WORKDIR)/$(BINDIR_AMIGAOS_TESTS)/overload1"

clean:
	rm -rf $(EMUDIR)/$(ASMDIR_AMIGAOS_TESTS)
	rm -rf $(EMUDIR)/$(OBJDIR_AMIGAOS_TESTS)
	rm -rf $(EMUDIR)/$(BINDIR_AMIGAOS_TESTS)
	cd mod1 ; make clean
	cd subtest11 ; make clean

dirs:
	mkdir -p $(EMUDIR)/$(ASMDIR_AMIGAOS_TESTS)
	mkdir -p $(EMUDIR)/$(OBJDIR_AMIGAOS_TESTS)
	mkdir -p $(EMUDIR)/$(BINDIR_AMIGAOS_TESTS)

