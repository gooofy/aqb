REM FILE i/o test 3 (write/input)

OPTION EXPLICIT

DIM AS BYTE     mybyte1, mybyte2
DIM AS UBYTE    myubyte1, myubyte2
DIM AS INTEGER  myint1, myint2
DIM AS UINTEGER myuint1, myuint2
DIM AS LONG     mylong1, mylong2
DIM AS ULONG    myulong1, myulong2
DIM AS SINGLE   myf1, myf2
DIM AS STRING   mys1, mys2

OPEN "hubba.txt" FOR OUTPUT AS #1

READ mybyte1, myubyte1, myint1, myuint1, mylong1, myulong1
READ myf1, mys1
WRITE #1, mybyte1, myubyte1, myint1, myuint1, mylong1, myulong1
WRITE #1, myf1, mys1
REM TRACE mybyte1, myubyte1, myint1, myuint1, mylong1, myulong1
REM TRACE myf1, mys1
CLOSE 1

OPEN "hubba.txt" FOR INPUT AS #3
INPUT #3, mybyte2, myubyte2, myint2, myuint2, mylong2, myulong2
REM TRACE "----> mybyte2=";mybyte2
ASSERT mybyte1=mybyte2
ASSERT myubyte1=myubyte2
ASSERT myint1=myint2
ASSERT myuint1=myuint2
ASSERT mylong1=mylong2
ASSERT myulong1=myulong2
INPUT #3, myf2, mys2
ASSERT myf1=myf2
ASSERT mys1=mys2
CLOSE #3

DATA 42, 255, 32000, 65000, -1000000, 2000000
DATA 23.42, "this is a test"

