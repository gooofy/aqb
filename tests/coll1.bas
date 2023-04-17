'
' Collections Test 1: ArrayList class, part 1
'

OPTION EXPLICIT

IMPORT Collections

'
' main
'

DIM s AS STRING

'
' try a simple string list
'

DIM o1 AS ArrayList PTR = NEW ArrayList()

'
' test Add, GetAt
'

o1->Add("foo")
o1->Add("bar")
o1->Add("baz")

s = o1->GetAt (0)
'TRACE s
ASSERT s = "foo"

s = o1->GetAt (1)
'TRACE s
ASSERT s = "bar"

s = o1->GetAt (2)
'TRACE s
ASSERT s = "baz"

'
' test Count
'

ASSERT o1->Count = 3

'
' test contains
'

DIM s2 AS STRING = "hubba"
ASSERT NOT o1->Contains(s2)

'
' test SetAt
'

o1->SetAt (1, s2)

'TRACE "3"
s = o1->GetAt (1)
'TRACE s
ASSERT s=s2

ASSERT o1->Contains(s2)

'
' test capacity getter
'

ASSERT o1->Capacity >= 3

'
' test IndexOf
'
ASSERT o1->IndexOf (s2)=1
ASSERT o1->IndexOf (s2, 0, 1)=-1
ASSERT o1->IndexOf (s2, 0, 2)=1
ASSERT o1->IndexOf (s2, 2)=-1
ASSERT o1->IndexOf (s2, 1)=1
ASSERT o1->IndexOf ("not there")=-1

'
' test Clear
'

o1->Clear()
ASSERT o1->Count=0

'
' test Add again, this time cause several capacity upgrades
'

FOR i AS INTEGER = 1 TO 42
    o1->Add("BLUBBER")
NEXT i

ASSERT o1->Count=42
FOR i AS INTEGER = 0 TO 41
    s = o1->GetAt(i)
    'TRACE i, s
    ASSERT s="BLUBBER"
NEXT i

ASSERT o1->Capacity >= 42

'
' test capacity setter
'

o1->Capacity=42
ASSERT o1->Capacity=42

