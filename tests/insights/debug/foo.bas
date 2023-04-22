'
' dyn array list/iterator interface test
'

OPTION EXPLICIT

DIM AS INTEGER a( 9)

' test SetAt/GetAt

FOR i AS INTEGER = 0 TO 9
    a.SetAt(i, i*i)
NEXT i

FOR i AS INTEGER = 0 TO 9
    DIM j AS INTEGER = a.GetAt(i)
    'TRACE j
    ASSERT j = i*i
NEXT i

' test Count, Capacity

' TRACE a.Count
ASSERT a.Count = 10
ASSERT a.Capacity = 10

' test enumeration

DIM e AS IEnumerator PTR

e = a.GetEnumerator()


