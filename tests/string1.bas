'
' test string duplication
'

OPTION EXPLICIT

' clib/exec_protos.h:VOID CopyMem( CONST APTR source, APTR dest, ULONG size );
DECLARE SUB CopyMem (src AS VOID PTR, dst AS VOID PTR, size AS ULONG) LIB -624 SysBase (a0, a1, d0)

FUNCTION STRDUP (s AS STRING) AS STRING

    DIM l AS ULONG = LEN(s)
    DIM r AS STRING = ALLOCATE (l+1)

    CopyMem s, r, l

    STRDUP = r

END FUNCTION

DIM s AS STRING = "ABC"

DIM r AS STRING

r = STRDUP(s)

' PRINT r, s

ASSERT r[0] = 65
ASSERT r[1] = 66
ASSERT r[2] = 67

ASSERT s[0] = 65
ASSERT s[1] = 66
ASSERT s[2] = 67

