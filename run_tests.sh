TESTS="cond1.bas cond2.bas cond3.bas cond4.bas exp1.bas exp2.bas exp4.bas exp5.bas extcall1.bas hello.bas opint.bas oplong.bas opsingle.bas opbool.bas subtest1.bas subtest2.bas var1.bas var2.bas win1.bas scopes1.bas scopes2.bas types1.bas types2.bas types3.bas types4.bas while1.bas constprop.bas"

pushd src/compiler

aqb() {

    if ./runaqb.sh $1 ; then
        return 0
    else
        echo "*** FAILED $1"
        return 1
    fi
}

for t in ${TESTS}; do

    echo $t

    aqb ../../tests/$t || exit 1

    # ./runaqb.sh ../../tests/$t || exit 1
done

popd

