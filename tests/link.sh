#!/bin/bash

EXAMPLE=foo

IRA='/home/guenter/projects/amiga/ira/src/ira/ira'

AS_AMIGAOS='/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-as'
VASM='/home/guenter/projects/amiga/vasm/src/vasm/vasmm68k_std'
LD_AMIGAOS='/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-ld'
OBJDUMP_AMIGAOS='/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-objdump'
STRIP_AMIGAOS='/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-strip'

#
# cleanup
#

rm -f foo.s foo.o foo foo.dump foo.asm foo_gnu.o foo_gnu.bin foo_gnu.dump foo.log foo_vasm.list aqb.log
rm -f /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/*.uaem


#
# AQB
#

#../target/x86_64-linux/bin/aqb -v -L ../src/lib -d _aqb -a foo.s -B foo.S -p foo.o -o foo ${EXAMPLE}.bas || exit
../target/x86_64-linux/bin/aqb -v -L ../src/lib -d _brt -a foo.s -B foo.S -p foo.o -o foo ${EXAMPLE}.bas || exit

# analyze object or load file produced by aqb's internal assembler/linker

/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-objdump -x -d -r foo.o &>foo.dump
echo ${IRA} -a foo
${IRA} -a foo
cp foo /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/

# create a object file using vasm for comparison

echo ${VASM}  foo.S -no-opt -Fhunk -o foo_vasm.o
${VASM} foo.S -no-opt -L foo_vasm.list -Fhunk -o foo_vasm.o
${LD_AMIGAOS} ../src/lib/_brt/startup.o foo_vasm.o ../src/lib/_brt/_brt.a -o foo_vasm.bin
${OBJDUMP_AMIGAOS} -x -d -r foo_vasm.o &>foo_vasm.dump
cp foo_vasm.bin /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/

# create a load file using GNU as / ld for comparison

${AS_AMIGAOS}    -march=68000 -mcpu=68000 foo.s -o foo_gnu.o
${LD_AMIGAOS}    ../src/lib/_brt/startup.o foo_gnu.o ../src/lib/_brt/_brt.a -o foo_gnu.bin
# ${STRIP_AMIGAOS} foo_gnu.bin
cp foo_gnu.bin /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/

${OBJDUMP_AMIGAOS} -x -d -r foo_gnu.o &>foo_gnu.dump
#${IRA} -a foo_gnu.bin

#
# run binary in vamos
#

# echo
# echo "vamos foo"
# vamos foo

