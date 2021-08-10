#!/bin/bash

EXAMPLE=gfx1

ASMProsrc='/home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/foo.asm'

../../target/x86_64-linux/bin/aqb -v -O -L ../../src/lib -A ${ASMProsrc}.tmp -o foo.s ${EXAMPLE}.bas
# ../../target/x86_64-linux/bin/aqb -v -L ../../src/lib -A ${ASMProsrc}.tmp -o foo.s ${EXAMPLE}.bas

cat ../../src/lib/minbrt/prolog.asm ${ASMProsrc}.tmp ../../src/lib/minbrt/minbrt.asm ../../src/lib/minbrt/math.s >${ASMProsrc}

rm -f ${ASMProsrc}.tmp

echo "${ASMProsrc} written."

