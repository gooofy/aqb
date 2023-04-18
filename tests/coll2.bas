'
' Collections Test 2: ArrayList class, part 2
'

OPTION EXPLICIT

IMPORT Collections

'
' main
'

DIM s AS STRING
DIM i AS INTEGER

'
' try a simple list of integers
'

DIM o1 AS ArrayList PTR = NEW ArrayList()

'
' test Add, GetAt
'

o1->Add(1)
o1->Add(9)
o1->Add(10)
o1->Add(11)

i = o1->GetAt (0)
'TRACE i
ASSERT i = 1

'
' test Count
'

ASSERT o1->Count = 4

'
' test contains
'

ASSERT o1->Contains(9)
ASSERT NOT o1->Contains(42)

'
' test ToString
'

s = o1->ToString()
'TRACE s
ASSERT s="ArrayList[ 1,  9,  10, ...]"


