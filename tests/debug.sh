#!/bin/bash

EXAMPLE=foo

ASMProsrc='/home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/foo.asm'

#../target/x86_64-linux/bin/aqb -O -v -L ../src/lib -d _brt -A ${ASMProsrc}.tmp -o foo.s ${EXAMPLE}.bas
#../target/x86_64-linux/bin/aqb -v -L ../src/lib -d _brt -A ${ASMProsrc}.tmp -o foo.hunk ${EXAMPLE}.bas || exit 1
../target/x86_64-linux/bin/aqb -v -L ../src/lib -d _aqb -A ${ASMProsrc}.tmp -o foo.hunk ${EXAMPLE}.bas || exit 1

cat ../src/lib/minbrt/prolog.asm ${ASMProsrc}.tmp ../src/lib/minbrt/minbrt.asm ../src/lib/minbrt/math.s >${ASMProsrc}

rm -f ${ASMProsrc}.tmp

echo "${ASMProsrc} written."

/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-objdump -x -d -r foo.hunk

