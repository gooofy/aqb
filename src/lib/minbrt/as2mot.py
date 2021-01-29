#!/usr/bin/env python3

import os
import sys
import re

BRANCH_TABLE = {
    'jra' : 'bra',
    'jhi' : 'bhi',
    'jls' : 'bls',
    'jhs' : 'bhs',
    'jlo' : 'blo',
    'jne' : 'bne',
    'jeq' : 'beq',
    'jvc' : 'bvc',
    'jvs' : 'bvs',
    'jpl' : 'bpl',
    'jmi' : 'bmi',
    'jge' : 'bge',
    'jlt' : 'blt',
    'jgt' : 'bgt',
    'jle' : 'ble',
}

CONVERSION_PATTERNS = [
     # move.l d0,(-368,a5)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+(?P<r0>[ad0-7]+),\((?P<off>[+\-0-9]+),(?P<r1>[ad0-7]+)\)$'), '\t\g<mn>\t\g<r0>, \g<off>(\g<r1>)'),
     # link.w a5,#-12
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+(?P<r0>[ad0-7]+),#(?P<imm>[+\-0-9]+)$'), '\t\g<mn>\t\g<r0>, #\g<imm>'),
     # clr.l (-4,a5)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+\((?P<off>[+\-0-9]+),(?P<r0>[ad0-7]+)\)$'), '\t\g<mn>\t\g<off>(\g<r0>)'),
     # subq.l #1,d0
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+#(?P<imm>[+\-0-9]+),(?P<r1>[ad0-7sp]+)$'), '\t\g<mn>\t#\g<imm>, \g<r1>'),
     # and.l #0xFFFF,d0
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+#(?P<imm>0x[+\-a-fA-F0-9]+),(?P<r1>[ad0-7sp]+)$'), '\t\g<mn>\t#\g<imm>, \g<r1>HEXHEX'),
     # jra .L2
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+(?P<label>[a-zA-Z0-9_\.]+)$'), '\t\g<mn>\t\g<label>'),
     # add.l d0,a0
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+(?P<r0>[ad0-7]+),(?P<r1>[ad0-7]+)$'), '\t\g<mn>\t\g<r0>, \g<r1>'),
     # move.l (12,a5),d0
     # movem.l (-28,a5),d2/d3/a6
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+\((?P<off>[+\-0-9]+),(?P<r0>[ad0-7sp]+)\),(?P<r1>[ad0-7sp/]+)$'), '\t\g<mn>\t\g<off>(\g<r0>), \g<r1>'),
     # move.b (a0),(-9,a5)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+\((?P<r0>[ad0-7]+)\),\((?P<off>[+\-0-9]+),(?P<r1>[ad0-7]+)\)$'), '\t\g<mn>\t(\g<r0>), \g<off>(\g<r1>)'),
     # move.l _g_positiveExpThreshold,(-10,a5)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+(?P<label>[a-zA-Z0-9_\.]+),\((?P<off>[+\-0-9]+),(?P<r1>[ad0-7]+)\)$'), '\t\g<mn>\t\g<label>, \g<off>(\g<r1>)'),
     # move.l #.LC0,d0
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+#(?P<label>\.L[a-zA-Z0-9_\.]+),(?P<r1>[ad0-7sp]+)$'), '\t\g<mn>\t#\g<label>, \g<r1>'),
     # move.l #.LC0,(-4,a5)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+#(?P<label>[a-zA-Z0-9_\.]+),\((?P<off>[+\-0-9]+),(?P<r1>[ad0-7]+)\)$'), '\t\g<mn>\t#\g<label>, \g<off>(\g<r1>)'),
     # pea .LC1
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+(?P<label>[a-zA-Z0-9_\.]+)$'), '\t\g<mn>\t\g<label>'),
     # move.b (a1),d0
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+\((?P<r0>[ad0-7sp]+)\),(?P<r1>[ad0-7sp]+)$'), '\t\g<mn>\t(\g<r0>), \g<r1>'),
     # move.b d0,(a0)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+(?P<r0>[ad0-7]+),\((?P<r1>[ad0-7]+)\)$'), '\t\g<mn>\t\g<r0>, (\g<r1>)'),
     # move.b (-9,a5),(a0)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+\((?P<off>[+\-0-9]+),(?P<r0>[ad0-7sp]+)\),\((?P<r1>[ad0-7sp]+)\)$'), '\t\g<mn>\t\g<off>(\g<r0>), (\g<r1>)'),
     # move.l (-4,a5),_g_mem
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+\((?P<off>[+\-0-9]+),(?P<r0>[ad0-7sp]+)\),(?P<label>[a-zA-Z0-9_\.]+)$'), '\t\g<mn>\t\g<off>(\g<r0>), \g<label>'),
     # addq.l #1,(-4,a5)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+#(?P<imm>[+\-0-9]+),\((?P<off>[+\-0-9]+),(?P<r1>[ad0-7]+)\)$'), '\t\g<mn>\t#\g<imm>, \g<off>(\g<r1>)'),
     # nop
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)$'), '\t\g<mn>'),
     # unlk a5
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+(?P<r1>[ad0-7]+)$'), '\t\g<mn>\t\g<r1>'),
     # move.b #32,(a0)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+#(?P<imm>[+\-0-9]+),\((?P<r1>[ad0-7]+)\)$'), '\t\g<mn>\t#\g<imm>, (\g<r1>)'),
     # move.w #1,_do_resume
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+#(?P<imm>[+\-0-9]+),(?P<label>[a-zA-Z0-9_\.]+)$'), '\t\g<mn>\t#\g<imm>, \g<label>'),
     # clr.b (a0)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+\((?P<r1>[ad0-7]+)\)$'), '\t\g<mn>\t(\g<r1>)'),
     # move.l (16,a5),-(sp)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+\((?P<off>[+\-0-9]+),(?P<r0>[ad0-7]+)\),-\((?P<r1>[ad0-7sp]+)\)$'), '\t\g<mn>\t\g<off>(\g<r0>), -(\g<r1>)'),
     # move.l d0,-(sp)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+(?P<r0>[ad0-7]+),-\((?P<r1>[ad0-7sp]+)\)$'), '\t\g<mn>\t\g<r0>, -(\g<r1>)'),
     # pea 1.w
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+(?P<addr>[0-7]+)\.(?P<w>[wlWL])$'), '\t\g<mn>\t\g<addr>.\g<w>'),
     # clr.l -(sp)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+-\((?P<r1>[ad0-7sp]+)\)$'), '\t\g<mn>\t-(\g<r1>)'),
     # move.l (8,a5),(-12,a5)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+\((?P<off>[+\-0-9]+),(?P<r0>[ad0-7sp]+)\),\((?P<off2>[+\-0-9]+),(?P<r1>[ad0-7sp]+)\)$'), '\t\g<mn>\t\g<off>(\g<r0>), \g<off2>(\g<r1>)'),
     # move.l _SysBase,d0
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+(?P<label>[a-zA-Z0-9_\.]+),(?P<r1>[ad0-7]+)$'), '\t\g<mn>\t\g<label>, \g<r1>'),
     # move.l d0,_SysBase
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+(?P<r1>[ad0-7]+),(?P<label>[a-zA-Z0-9_\.]+)$'), '\t\g<mn>\t\g<r1>, \g<label>'),
     # jsr a6@(-0x270:W)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+(?P<r0>[ad0-7sp]+)@\((?P<off>[+\-0-9xa-fA-F]+):(?P<w>[wlWL])\)$'), '\t\g<mn>\t\g<off>(\g<r0>)HEXHEX'),
     # jmp %pc@(2,d0:w)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+%pc@\((?P<off>[+\-0-9xa-fA-F]+),(?P<r0>[ad0-7sp]+):(?P<w>[wlWL])\)$'), '\t\g<mn>\t\g<off>(pc, \g<r0>)'),
     # move.l (sp)+,d2
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+\((?P<r0>[ad0-7sp]+)\)\+,(?P<r1>[ad0-7sp]+)$'), '\t\g<mn>\t(\g<r0>)+, \g<r1>'),
     # movem.l a6/d3/d2,-(sp)
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+(?P<regs>[ad0-7/]+),-\((?P<r1>[ad0-7sp]+)\)$'), '\t\g<mn>\t\g<regs>, -(\g<r1>)'),
     # movem.l (sp)+,d2/d3/a6
     (re.compile(r'^\s+(?P<mn>[a-zA-Z][a-zA-Z.]+)\s+\((?P<r1>[ad0-7sp]+)\)\+,(?P<regs>[ad0-7/]+)$'), '\t\g<mn>\t(\g<r1>)+, \g<regs>'),

     # .lcomm _g_positiveExpThreshold,4
     (re.compile(r'^\.lcomm\s+(?P<label>[a-zA-Z_.0-9]+),(?P<size>[0-9]+)$'), '\g<label>:\tDS.B\t\g<size>'),
     # .skip 4
     (re.compile(r'^\s+\.skip\s+(?P<size>[0-9]+)$'), '\tDS.B\t\g<size>'),
     # .globl  __astr_itoa_ext
     (re.compile(r'^\s+\.globl\s+(?P<label>[a-zA-Z_.0-9]+)$'), '\tXDEF\t\g<label>'),
     # .ascii "TRUE\0"
     (re.compile(r'^\s+\.ascii\s+"(?P<str>[A-Za-z0-9 ,.?!*]*)\\0"$'), '\tDC.B\t\'\g<str>\', 0'),
     # .ascii "*** error: failed to open dos.library!\12\0"
     (re.compile(r'^\s+\.ascii\s+"(?P<str>[A-Za-z0-9 ,.?:;!\*]+)\\12\\0"$'), '\tDC.B\t\"\g<str>\", 10, 0'),
     # .ascii "\12\0"
     (re.compile(r'^\s+\.ascii\s+"\\12\\0"$'), '\tDC.B\t 10, 0'),
     # .ascii "\14\0"
     (re.compile(r'^\s+\.ascii\s+"\\14\\0"$'), '\tDC.B\t 12, 0'),
     # .ascii "*** unhandled runtime error code: \0"'
     (re.compile(r'^\s+\.ascii\s+"(?P<str>[A-Za-z0-9 ,.?:;!\*]+)\\0"$'), '\tDC.B\t\"\g<str>\", 0'),

     (re.compile(r'^\#NO_APP'), ''),
     (re.compile(r'^\#APP'), ''),
     # | 136 "astr.c" 1
     (re.compile(r'^\|\s+[0-9]+\s+"'), '; |'),
     # .text
     (re.compile(r'^\s+.text'), '\tSECTION SECTIONNAME, CODE'),
     # .bss
     (re.compile(r'^\s+.bss'), '\tSECTION SECTIONNAME, BSS'),
     (re.compile(r'^\s+.align\s+2'), '\tEVEN'),
     (re.compile(r'^(?P<label>[a-zA-Z_.0-9]+):$'), '\g<label>:'),
     # (re.compile(), ''),
    ]


if len(sys.argv) != 2:
    print ("usage: %s <foo.asm>", sys.argv[0])
    sys.exit(1)

asmfn = sys.argv[1]

print ("%s: processing %s..." % (sys.argv[0], asmfn), file=sys.stderr)

scnt=0

with open (asmfn, 'r') as asmf:

    for line in asmf:

        l = line.rstrip()

        # FIXME: crude local label transformation
        l = re.sub(r"\.LC([0-9]+)", r"_gccLC\1", l)
        l = re.sub(r"\.L([0-9]+)", r"_gccL\1", l)

        matched = False
        for p, f in CONVERSION_PATTERNS:

            m = p.match(l)
            if m:
                # print ("*** MATCH ***: %s" % repr(m.groups()))

                s = m.expand(f)
                if ('HEXHEX' in s):
                    s = s.replace('HEXHEX','').replace('0x','$')

                if ('SECTIONNAME' in s):
                    s = s.replace('SECTIONNAME', 's%05d' % scnt)
                    scnt += 1

                for j, b in BRANCH_TABLE.items():
                    s = s.replace (j, b)

                # print ("; %s" % p)
                print (s)
                matched=True
                break

        if not matched:
            print ("*** FAIL: r'%s'" % l, file=sys.stderr)
            sys.exit(1)

