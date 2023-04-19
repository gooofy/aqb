'
' Collections Test 3: ArrayList iterators
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

FOR i = 1 TO 10
    o1->Add(i)
NEXT i

TRACE o1
ASSERT o1->Count = 10

'
' enumerate
'

DIM e AS IEnumerator PTR

e = o1->GetEnumerator()

