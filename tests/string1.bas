'
' test string duplication
'

OPTION EXPLICIT

' clib/exec_protos.h:VOID CopyMem( CONST APTR source, APTR dest, ULONG size );
DECLARE SUB CopyMem (BYVAL src AS VOID PTR, BYVAL dst AS VOID PTR, BYVAL size AS ULONG) LIB -624 SysBase (a0, a1, d0)

FUNCTION STRDUP (s AS STRING) AS STRING

    DIM l AS ULONG = LEN(s)
    DIM r AS STRING = ALLOCATE (l+1)

    CopyMem s, r, l+1

    STRDUP = r

END FUNCTION

DIM s AS STRING = "ABC"

' _debug_puts "s=" : _debug_puts s : _debug_putnl

DIM r AS STRING

r = STRDUP(s)

' _debug_puts "after STRDUP(s): r=" : _debug_puts r : _debug_puts ", s=" : _debug_puts s : _debug_putnl

ASSERT r[0] = 65
ASSERT r[1] = 66
ASSERT r[2] = 67

ASSERT s[0] = 65
ASSERT s[1] = 66
ASSERT s[2] = 67

