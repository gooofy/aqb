'
' Collections Test 3: CArrayList iterators
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

DIM o1 AS CArrayList PTR = NEW CArrayList()

o1->Add(42)

FOR i = 1 TO 10
    o1->Add(i)
NEXT i

'TRACE o1
'ASSERT o1->Count = 10

'
' enumerate
'

DIM e AS IEnumerator PTR

e = o1->GetEnumerator()

'TRACE "enumerating..."

DIM AS INTEGER cnt=0, sum=0

WHILE e->MoveNext()
    i = e->Current
    'TRACE "element: "; i
    cnt=cnt+1
    sum=sum+i
WEND

'TRACE "done. sum=";sum;", cnt=";cnt

ASSERT cnt=11
ASSERT sum=97

'
' test Reset
'

e->Reset()

cnt=0 : sum=0
WHILE e->MoveNext()
    i = e->Current
    'TRACE "element: "; i
    cnt=cnt+1
    sum=sum+i
WEND

'TRACE "done. sum=";sum;", cnt=";cnt

ASSERT cnt=11
ASSERT sum=97

