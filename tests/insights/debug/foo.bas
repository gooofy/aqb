'
' Collections Test 2: CArrayList class, part 2
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
ASSERT s="CArrayList[ 1,  9,  10, ...]"

'
' test IsReadOnly, IsFixedSize
'

ASSERT NOT o1->IsReadOnly
ASSERT NOT o1->IsFixedSize

'
' test Insert
'

o1->Insert (1, 2)
'TRACE o1->Count, o1->ToString()
ASSERT o1->Count = 5
ASSERT o1->Contains(2)
ASSERT o1->IndexOf(2)=1
ASSERT o1->IndexOf(11)=4

'
' test Remove
'

o1->Remove (9)
ASSERT o1->Count = 4
ASSERT o1->Contains(2)
ASSERT NOT o1->Contains(9)

'
' test RemoveAt
'
o1->RemoveAt(1)
s = o1->ToString()
'TRACE s
ASSERT NOT o1->Contains(2)
ASSERT o1->Count = 3
ASSERT o1->GetAt(0)=1
ASSERT o1->GetAt(1)=10
ASSERT o1->GetAt(2)=11

'
' test Clone()
'

DIM AS CArrayList PTR o2 = CAST(CArrayList PTR, o1->Clone())

' TRACE o2->ToString()

ASSERT NOT o2->Contains(2)
ASSERT o2->Count = 3
ASSERT o2->GetAt(0)=1
ASSERT o2->GetAt(1)=10
ASSERT o2->GetAt(2)=11
