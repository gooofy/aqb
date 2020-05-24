#!/bin/bash

VERBOSE=""

while [ "$#" -gt 0 ]; do
  case "$1" in
    -v) VERBOSE="-v"; shift;;
    -*) echo "unknown option: $1" >&2; exit 1;;
    *) break;;
  esac
done

if [ $# != 1 ] ; then
    echo "usage: $0 [-v] <foo.bas>"
    exit 1
fi

AQBSRC=$1

AQBBASE=`basename ${AQBSRC} .bas`

WORKDIR=`dirname ${AQBSRC}`

AQBBIN="${WORKDIR}/${AQBBASE}.bin"
AQBASM="${WORKDIR}/${AQBBASE}.s"
AQBOBJ="${WORKDIR}/${AQBBASE}.o"

./aqb $VERBOSE $1 || exit 2

m68k-amigaos-as ${AQBASM} -o ${AQBOBJ} || exit 3

m68k-amigaos-ld ../alib/startup.o ${AQBOBJ} ../alib/_alib.a -o ${AQBBIN} || exit 4

echo "${AQBBIN} created."

cp ${AQBBIN} /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/
cp ${AQBBIN} /home/guenter/media/emu/amiga/a500/hdd/system/x/
# m68k-amigaos-objdump -S -d -x ${AQBBIN}

