#!/bin/bash

pushd tests
make clean
popd

rm -f /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/*.bin
rm -f /home/guenter/media/emu/amiga/a500/hdd/system/x/*.bin

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

