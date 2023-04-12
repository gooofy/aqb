'
' OOP test 15: ToString(), Equals(), GetHashCode()
'

OPTION EXPLICIT

' myb inherits from Object implicitly
CLASS myb

    field1 AS INTEGER

    DECLARE CONSTRUCTOR (BYVAL initValue AS INTEGER)

    DECLARE SUB store (BYVAL i AS INTEGER)
    DECLARE FUNCTION retrieve () AS INTEGER

END CLASS

CONSTRUCTOR myb (BYVAL initValue AS INTEGER)
    field1 = initValue
END CONSTRUCTOR

SUB myb.store (BYVAL i AS INTEGER)
    field1 = i
END SUB

FUNCTION myb.retrieve() AS INTEGER
    RETURN field1
END FUNCTION

' myc inherits from myb but overrides ToString(), Equals(), GetHashCode()

CLASS myc EXTENDS myb

    DECLARE CONSTRUCTOR (BYVAL initValue AS INTEGER)

    ' override ToString()
    DECLARE VIRTUAL FUNCTION ToString() AS STRING
    DECLARE VIRTUAL FUNCTION Equals(BYREF obj2 AS Object) AS BOOLEAN
    DECLARE VIRTUAL FUNCTION GetHashCode () AS ULONG

END CLASS

CONSTRUCTOR myc (BYVAL initValue AS INTEGER)
    BASE.CONSTRUCTOR(initValue)
END CONSTRUCTOR

FUNCTION myc.ToString() AS STRING

    RETURN "<" + STR$(field1) + ">"

END FUNCTION

FUNCTION myc.Equals(BYREF obj2 AS Object) AS BOOLEAN

    DIM myc2 AS myc PTR = CAST (myc PTR, @obj2)

    RETURN myc2->retrieve() = retrieve()

END FUNCTION

FUNCTION myc.GetHashCode () AS ULONG
    RETURN retrieve()
END FUNCTION

'
' main
'

DIM s AS STRING

DIM o1 AS Object PTR = NEW Object()
DIM o2 AS myb PTR = NEW myb(23)
DIM o3 AS myc PTR = NEW myc(23)

'
' test ToString()
'

s = o1->ToString()

'TRACE "o1=";o1;", s=";s
ASSERT INSTR(, s, "obj@")=1

s = o2->ToString()
'TRACE "o2=";o2;", s=";s
ASSERT INSTR(, s, "obj@")=1

s = o3->ToString()
'TRACE "o3=";o3;", s=";s
ASSERT s = "< 23>"

'
' test Equals()
'

'TRACE "o3 <> o2 -> ";o2<>o3
ASSERT o2 <> o3
' TRACE "o3->Equals(o2) -> ";o3->Equals(*o2)
ASSERT o3->Equals(*o2)

'
' test GetHashCode()
'

ASSERT o1->GetHashCode() = CAST (ULONG, o1)
ASSERT o2->GetHashCode() = CAST (ULONG, o2)
ASSERT o3->GetHashCode() = 23

