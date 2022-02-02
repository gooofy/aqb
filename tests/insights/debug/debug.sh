#!/bin/bash

EXAMPLE=foo

AQB=../../../target/x86_64-linux/bin/aqb
AQBLIBS=../../../src/lib
AQBRT=_aqb
AQBFLAGS="-v"

ASMProsrc='/home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/foo.asm'
OBJDUMPOUT='dump.txt'

rm -f aqb.log ${ASMProsrc}.tmp foo.hunk foo.s ${ASMProsrc}

${AQB} ${AQBFLAGS} -L ${AQBLIBS} -d ${AQBRT} -A ${ASMProsrc}.tmp -o foo.hunk ${EXAMPLE}.bas || exit 1

cat ${AQBLIBS}/minbrt/prolog.asm ${ASMProsrc}.tmp ${AQBLIBS}/minbrt/minbrt.asm ${AQBLIBS}/minbrt/math.s >${ASMProsrc}

rm -f ${ASMProsrc}.tmp

echo "${ASMProsrc} written."

/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-objdump -x -d -r foo.hunk 2>&1 >${OBJDUMPOUT}

echo "${OBJDUMPOUT} written."

