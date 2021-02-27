#!/bin/bash

EXAMPLE=foo

IRA='/home/guenter/projects/amiga/ira/src/ira/ira'

AS_AMIGAOS='/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-as'
LD_AMIGAOS='/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-ld'
STRIP_AMIGAOS='/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-strip'

#
# cleanup
#

rm -f foo.s foo foo.dump foo.asm foo_gnu.o foo_gnu.bin foo_gnu.dump foo.log

#
# AQB
#

../target/x86_64-linux/bin/aqb -v -L ../src/lib -d _brt -a foo.s -o foo ${EXAMPLE}.bas || exit

#
# analyze load file produced by aqb's internal assembler/linker
#

/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-objdump -x -d -r foo &>foo.dump
echo ${IRA} -a foo
${IRA} -a foo
cp foo /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/

# create a load file using GNU as / ld for comparison

${AS_AMIGAOS}    -march=68000 -mcpu=68000 foo.s -o foo_gnu.o
${LD_AMIGAOS}    ../src/lib/_brt/startup.o foo_gnu.o ../src/lib/_brt/_brt.a -o foo_gnu.bin
# ${STRIP_AMIGAOS} foo_gnu.bin

/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-objdump -x -d -r foo_gnu.bin &>foo_gnu.dump
#${IRA} -a foo_gnu.bin

#
# run binary in vamos
#

# echo
# echo "vamos foo"
# vamos foo

