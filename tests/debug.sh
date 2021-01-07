#!/bin/bash

EXAMPLE=cond4

ASMProsrc='/home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/foo.asm'

../target/x86_64-linux/bin/aqb -O -v -L ../src/lib -d _brt -A ${ASMProsrc}.tmp -o foo.s ${EXAMPLE}.bas

cat ../src/lib/minbrt/prolog.asm ${ASMProsrc}.tmp ../src/lib/minbrt/minbrt.asm ../src/lib/minbrt/math.s >${ASMProsrc}

rm -f ${ASMProsrc}.tmp

