#!/bin/bash

EXAMPLE=foo

AQB=../../../target/x86_64-linux/bin/aqb
AQBLIBS=../../../src/lib
AQBRT=_aqb
AQBFLAGS="-v"

OBJDUMP_AMIGAOS='/home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-objdump'

#
# cleanup
#

rm -f foo.s foo.o foo foo.dump foo.asm foo_gnu.o foo_gnu.bin foo_gnu.dump foo.log foo_vasm.list aqb.log foo.S  foo_vasm.dump  foo_vasm.o
rm -f /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/*.uaem


#
# AQB
#

valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt \
    ${AQB} ${AQBFLAGS} -L ${AQBLIBS} -d ${AQBRT} -a foo.s -B foo.S -p foo.o -o foo ${EXAMPLE}.bas

# analyze object or load file produced by aqb's internal assembler/linker

# /home/guenter/projects/amiga/amiga-gcc/bin/m68k-amigaos-objdump -x -d -r foo.o &>foo.dump
