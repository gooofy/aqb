#!/bin/bash

EXAMPLE=foo

IRA='/home/guenter/projects/amiga/ira/src/ira/ira'
AQB="../../target/x86_64-linux/bin/aqb"
AS_AMIGAOS='/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-as'
LD_AMIGAOS='/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-ld'
STRIP_AMIGAOS='/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-strip'
OBJDUMP_AMIGAOS='/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-objdump'

#
# cleanup
#

rm -f ${EXAMPLE}.s ${EXAMPLE} ${EXAMPLE}.dump ${EXAMPLE}.asm ${EXAMPLE}_gnu.o ${EXAMPLE}_gnu.bin ${EXAMPLE}_gnu.dump ${EXAMPLE}.log

#
# AQB
#

${AQB} -v -L ../../src/lib -a ${EXAMPLE}.s -o ${EXAMPLE} ${EXAMPLE}.bas || exit
cp ${EXAMPLE} /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/

#
# analyze load file produced by aqb's internal assembler/linker
#

${OBJDUMP_AMIGAOS} -x -d -r ${EXAMPLE} &>${EXAMPLE}.dump
echo ${IRA} -a ${EXAMPLE}
${IRA} -a ${EXAMPLE}

# create a load file using GNU as / ld for comparison

${AS_AMIGAOS} -march=68000 -mcpu=68000 ${EXAMPLE}.s -o ${EXAMPLE}_gnu.o
${LD_AMIGAOS} ../../src/lib/_brt/startup.o ${EXAMPLE}_gnu.o ../../src/lib/_brt.a ../../src/lib/_aqb.a -o ${EXAMPLE}_gnu.bin
cp ${EXAMPLE}_gnu.bin /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/

${OBJDUMP_AMIGAOS} -x -d -r ${EXAMPLE}_gnu.bin &>${EXAMPLE}_gnu.dump
#${IRA} -a ${EXAMPLE}_gnu.bin

#
# run binary in vamos
#

# echo
# echo "vamos ${EXAMPLE}"
# vamos ${EXAMPLE}

