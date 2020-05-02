#!/bin/bash

pushd src/compiler

aqb() {

    if ./runaqb.sh $1 ; then
        return 0
    else
        echo "*** FAILED $1"
        return 1
    fi
}

for tf in ../../tests/*.bas ; do

    t=`basename $tf`

    echo $t

    aqb ../../tests/$t || exit 1

done

popd

