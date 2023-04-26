'
' ExecList test 1
'

OPTION EXPLICIT

IMPORT Collections

DIM AS CExecList a = CExecList(NT_USER)

'
' test AddNode / GetNodeAt
'

FOR i AS INTEGER = 0 TO 9
    a.AddNode(new CExecNode (i, i, 0, "hubba") )
NEXT i

FOR i AS INTEGER = 0 TO 9
    DIM n AS CExecNode PTR = a.GetNodeAt(i)
    TRACE n->Type, n->Name
    ASSERT n->Type = i
    ASSERT n->Name = "hubba"
NEXT i

