'
' coll5: ExecList test part 2
'

OPTION EXPLICIT

IMPORT Collections

DIM AS CExecList a = CExecList(NT_USER)

'
' test AddNode / GetNodeAt
'

FOR i AS INTEGER = 0 TO 9
    a.AddNode(new CExecNode (i*i, i, 0, "hubba") )
NEXT i

FOR i AS INTEGER = 0 TO 9
    DIM n AS CExecNode PTR = a.GetNodeAt(i)
    'TRACE CAST(INTEGER, n->Value), n->Type, n->Name

    ' test node property getters

    ASSERT n->Type = i
    ASSERT n->Name = "hubba"
    ASSERT n->Value = i*i
    ASSERT n->Pri = 0

    ' test node property setters

    n->Value = i
    n->Type = i*i
    n->Pri = 0
    n->Name = "blubber"
NEXT i

' check if node property setters worked

FOR i AS INTEGER = 0 TO 9
    DIM n AS CExecNode PTR = a.GetNodeAt(i)

    ASSERT n->Type = i*i
    ASSERT n->Name = "blubber"
    ASSERT n->Value = i
    ASSERT n->Pri = 0
NEXT i

'
' test Clone()
'

DIM AS CExecList PTR b

b = CAST (CExecList PTR, a.Clone())

ASSERT b->Count = 10

FOR i AS INTEGER = 0 TO 9
    DIM n AS CExecNode PTR = b->GetNodeAt(i)

    ASSERT n->Type = i*i
    ASSERT n->Name = "blubber"
    ASSERT n->Value = i
    ASSERT n->Pri = 0
NEXT i

'
' test RemoveAll
'

b->RemoveAll()
ASSERT b->Count=0

