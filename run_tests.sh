TESTS="cond1.bas cond2.bas cond3.bas exp1.bas exp2.bas exp4.bas exp5.bas extcall1.bas hello.bas opint.bas oplong.bas opsingle.bas subtest1.bas subtest2.bas var1.bas win1.bas"

pushd src/compiler

for t in ${TESTS}; do

    echo $t

    ./runaqb.sh ../../tests/$t || exit 1
    
done

popd

