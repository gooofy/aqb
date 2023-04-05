#!/bin/bash

EXAMPLE=foo

AQB=../../../target/x86_64-linux/bin/aqb
AQBLIBS=../../../src/lib
# AQBRT=_aqb
AQBRT=_brt
AQBFLAGS="-v"
OBJDUMP="/opt/amiga/bin/m68k-amigaos-objdump"

ASMProsrctmp='foo.asm'
ASMProsrc="${AMIGA_X}/foo.asm"
OBJDUMPOUT='dump.txt'

rm -f aqb.log ${ASMProsrctmp} foo.hunk foo.s ${ASMProsrc}

echo ${AQB} ${AQBFLAGS} -L ${AQBLIBS} -d ${AQBRT} -A ${ASMProsrctmp} -o foo.hunk ${EXAMPLE}.bas || exit 1
${AQB} ${AQBFLAGS} -L ${AQBLIBS} -d ${AQBRT} -A ${ASMProsrctmp} -o foo.hunk ${EXAMPLE}.bas || exit 1

cat ${AQBLIBS}/minbrt/prolog.asm ${ASMProsrctmp} ${AQBLIBS}/minbrt/minbrt.asm ${AQBLIBS}/minbrt/math.s >${ASMProsrc}

#rm -f ${ASMProsrctmp}

echo "${ASMProsrc} written."

${OBJDUMP} -x -d -r foo.hunk 2>&1 >${OBJDUMPOUT}

echo "${OBJDUMPOUT} written."

cp foo.hunk ${AMIGA_X}
