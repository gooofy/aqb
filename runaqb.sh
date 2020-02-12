#!/bin/bash

if [ $# != 1 ] ; then
    echo "usage: $0 <foo.bas>"
    exit 1
fi

TIGERSRC=$1

TIGERBASE=`basename ${TIGERSRC} .bas`

WORKDIR=`dirname ${TIGERSRC}`

EXE="${WORKDIR}/${TIGERBASE}"

./aqb $1 || exit 2

m68k-amigaos-as ${1}.s -o ${1}.o || exit 3

# m68k-amigaos-ld ../alib/_alib.o ${1}.o -o ${EXE} || exit 4

m68k-amigaos-ld ../alib/startup.o ${1}.o ../alib/_alib.a -o ${EXE} || exit 4

echo "${EXE} created."

cp ${EXE} /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/
cp ${EXE} /home/guenter/media/emu/amiga/a500/hdd/system/x/
# m68k-amigaos-objdump -S -d -x ${EXE}

