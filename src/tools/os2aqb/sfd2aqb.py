#!/bin/env python3

#
# _very_ crude converter amiga os include -> AQB
#

import re
import sys
import io

C2PY = {"ULONG": "ULONG",
        "CONST ULONG *": "ULONG PTR",
        "ULONG *": "ULONG PTR",
        "LONG": "LONG",
        "CONST struct BitMap *": "BitMap PTR",
        "PLANEPTR": "VOID PTR",
        "CONST PLANEPTR": "VOID PTR",
        "CONST_STRPTR": "String",
        "WORD": "INTEGER",
        "CONST UWORD *": "UINTEGER PTR",
        "UWORD *": "UINTEGER PTR",
        "UBYTE *": "UBYTE PTR",
        "CONST UBYTE *": "UBYTE PTR",
        "UWORD": "UINTEGER",
        "CONST struct TextFont *": "TextFont PTR",
        "APTR": "APTR",
        "CONST APTR": "VOID PTR",
        "struct AnimOb **": "AnimOb PTR PTR",
        "BOOL": "BOOLEAN",
        "Tag": "ULONG",
        "DisplayInfoHandle": "VOID PTR",
        "CONST DisplayInfoHandle": "VOID PTR",
        "CONST WORD *": "INTEGER PTR",
        }

PTRPATTERN = re.compile(r"^struct\W+(\w+)\W+\*")
CPTRPATTERN = re.compile(r"^CONST struct\W+(\w+)\W+\*")
CPTRPATTERN2 = re.compile(r"^const struct\W+(\w+)\W+\*")

def c2py (ty):

    if not ty:
        return "???"

    m = PTRPATTERN.match(ty)
    if m:
        return "%s PTR" % m.group(1)

    m = CPTRPATTERN.match(ty)
    if m:
        return "%s PTR" % m.group(1)

    m = CPTRPATTERN2.match(ty)
    if m:
        return "%s PTR" % m.group(1)

    return C2PY[ty]

# LONG BltBitMap(CONST struct BitMap * srcBitMap, LONG xSrc, LONG ySrc, struct BitMap * destBitMap, LONG xDest, LONG yDest, LONG xSize, LONG ySize, ULONG minterm, ULONG mask, PLANEPTR tempA) (a0,d0,d1,a1,d2,d3,d4,d5,d6,d7,a2)

#SIGPATTERN = re.compile(r"^(\w+)\W([^(]*)\(([^)]*)\)\W*\(([^)]*)\)")
SIGPATTERN = re.compile(r"^([^(]+)\(([^)]*)\)\W*\(([^)]*)\)")

input_stream = io.TextIOWrapper(sys.stdin.buffer, encoding='latin1')

libBase = '???'
baseType = '???'
offset = -30

for line in input_stream:

    if not line:
        continue

    line = line.strip()

    if line.startswith ('=='):

        if line.startswith ('==base '):
            libBase = line[7:]
            print ("REM libBase: %s" % libBase)
        elif line.startswith ('==basetype '):
            baseType = line[11:]
            print ("REM baseType: %s" % baseType)
    else:
        # print(line)

        m = SIGPATTERN.match (line)
        if m:
            # print ("*** MATCH %s" % m.group(1))
            # print ("          %s" % m.group(2))
            # print ("          %s" % m.group(3))

            ri = m.group(1).rfind(' ')
            returntype = m.group(1)[:ri]
            fname = m.group(1)[ri:]
            # print (returntype, ri, fname)

            args = m.group(2).split(',')
            regs = m.group(3)

            if m.group(1) == "VOID":
                sys.stdout.write ("DECLARE SUB %s (" % fname)
            else:
                sys.stdout.write ("DECLARE FUNCTION %s (" % fname)

            first=True
            for arg in args:

                if not arg:
                    continue

                if first:
                    first = False
                else:
                    sys.stdout.write (", ")

                ti = arg.rfind(' ')
                argname = arg[ti:].strip()
                sys.stdout.write ("BYVAL %s AS %s" % (argname, c2py(arg[:ti].strip())))

            if returntype == "VOID":
                sys.stdout.write (")")
            else:
                sys.stdout.write (") AS %s" % c2py(returntype))

            print (" LIB %d %s (%s)" % (offset, libBase, regs))
            offset -= 6
        else:
            print ("REM %s" % line)
            assert False


