OPTION EXPLICIT
OPTION PRIVATE

IMPORT OSExec

PUBLIC CLASS CArrayList IMPLEMENTS IList, ICloneable

    PUBLIC:

        DECLARE EXTERN CONSTRUCTOR (BYVAL capacity AS LONG = 0)

        DECLARE EXTERN VIRTUAL PROPERTY Capacity AS LONG
        DECLARE EXTERN VIRTUAL PROPERTY Capacity (BYVAL c AS LONG)

        ' IEnumerable

        DECLARE EXTERN VIRTUAL FUNCTION GetEnumerator () AS IEnumerator PTR

        ' ICollection

        DECLARE EXTERN VIRTUAL PROPERTY Count AS LONG

        ' IList

        DECLARE EXTERN VIRTUAL FUNCTION GetAt (BYVAL index AS LONG) AS ANY
        DECLARE EXTERN VIRTUAL SUB      SetAt (BYVAL index AS LONG, BYVAL obj AS ANY)

        DECLARE EXTERN VIRTUAL FUNCTION Add (BYVAL obj AS ANY) AS LONG

        DECLARE EXTERN VIRTUAL FUNCTION Contains (BYVAL value AS ANY) AS BOOLEAN

        DECLARE EXTERN VIRTUAL PROPERTY IsReadOnly AS BOOLEAN

        DECLARE EXTERN VIRTUAL PROPERTY IsFixedSize AS BOOLEAN

        DECLARE EXTERN VIRTUAL FUNCTION IndexOf (BYVAL value AS ANY, BYVAL startIndex AS LONG=0, BYVAL count AS LONG=-1) AS LONG

        DECLARE EXTERN VIRTUAL SUB Insert (BYVAL index AS LONG, BYVAL value AS ANY)

        DECLARE EXTERN VIRTUAL SUB Remove (BYVAL value AS ANY)

        DECLARE EXTERN VIRTUAL SUB RemoveAt (BYVAL index AS LONG)

        DECLARE EXTERN VIRTUAL SUB RemoveAll

        ' ICloneable

        DECLARE EXTERN VIRTUAL FUNCTION Clone () AS CObject PTR

    PRIVATE:

		AS ANY PTR    _items
        AS LONG       _size
        AS LONG       _capacity

END CLASS

PUBLIC CLASS CArrayListEnumerator IMPLEMENTS IEnumerator

    PUBLIC:

        DECLARE EXTERN CONSTRUCTOR (BYVAL list AS CArrayList PTR)

        DECLARE EXTERN VIRTUAL FUNCTION MoveNext() AS BOOLEAN
        DECLARE EXTERN VIRTUAL PROPERTY Current AS ANY
        DECLARE EXTERN VIRTUAL SUB      Reset()

    PRIVATE:

        AS CArrayList PTR _list
        AS LONG           _index
        AS ANY            _currentElement

END CLASS

' --------------------------------------------------------------------------------------------------------
' --
' -- Exec lists: OOP Wrapper
' --
' --------------------------------------------------------------------------------------------------------

PUBLIC TYPE ExecNodeAny
        AS Node    n
        AS ANY PTR enode ' points back to the CExecNode instance which owns this ExecNodeAny
        AS ANY     value
END TYPE

PUBLIC CLASS CExecNode


    PUBLIC:
        DECLARE EXTERN CONSTRUCTOR (BYVAL value AS ANY, BYVAL ln_Type AS UBYTE = NT_USER, BYVAL ln_Pri AS BYTE = 0, BYVAL ln_Name AS STRING = NULL)

        DECLARE EXTERN VIRTUAL PROPERTY ExecNode AS Node PTR

        DECLARE EXTERN VIRTUAL PROPERTY Type AS UBYTE
        DECLARE EXTERN VIRTUAL PROPERTY Type (t AS UBYTE)

        DECLARE EXTERN VIRTUAL PROPERTY Pri AS String
        DECLARE EXTERN VIRTUAL PROPERTY Pri (s AS String)

        DECLARE EXTERN VIRTUAL PROPERTY Name AS String
        DECLARE EXTERN VIRTUAL PROPERTY Name (s AS String)

    PRIVATE:
        AS ExecNodeAny n

END CLASS

PUBLIC CLASS CExecList IMPLEMENTS IList, ICloneable

    PUBLIC:

        DECLARE EXTERN CONSTRUCTOR (BYVAL lh_Type AS UBYTE)

        DECLARE EXTERN VIRTUAL PROPERTY ExecList AS List PTR

        DECLARE EXTERN VIRTUAL SUB      AddNode   (BYVAL n AS CExecNode PTR)
        DECLARE EXTERN VIRTUAL FUNCTION GetNodeAt (BYVAL index AS LONG) AS CExecNode PTR

        ' IEnumerable

        DECLARE EXTERN VIRTUAL FUNCTION GetEnumerator () AS IEnumerator PTR

        ' ICollection

        DECLARE EXTERN VIRTUAL PROPERTY Count AS LONG

        ' IList

        DECLARE EXTERN VIRTUAL FUNCTION GetAt (BYVAL index AS LONG) AS ANY
        DECLARE EXTERN VIRTUAL SUB      SetAt (BYVAL index AS LONG, BYVAL obj AS ANY)

        ' note: will always return 0 for CExecLists
        DECLARE EXTERN VIRTUAL FUNCTION Add (BYVAL obj AS ANY) AS LONG

        DECLARE EXTERN VIRTUAL FUNCTION Contains (BYVAL value AS ANY) AS BOOLEAN

        DECLARE EXTERN VIRTUAL PROPERTY IsReadOnly AS BOOLEAN

        DECLARE EXTERN VIRTUAL PROPERTY IsFixedSize AS BOOLEAN

        DECLARE EXTERN VIRTUAL FUNCTION IndexOf (BYVAL value AS ANY, BYVAL startIndex AS LONG=0, BYVAL count AS LONG=-1) AS LONG

        DECLARE EXTERN VIRTUAL SUB Insert (BYVAL index AS LONG, BYVAL value AS ANY)

        DECLARE EXTERN VIRTUAL SUB Remove (BYVAL value AS ANY)

        DECLARE EXTERN VIRTUAL SUB RemoveAt (BYVAL index AS LONG)

        DECLARE EXTERN VIRTUAL SUB RemoveAll

        ' ICloneable

        DECLARE EXTERN VIRTUAL FUNCTION Clone () AS CObject PTR

    PRIVATE:
        AS List l

END CLASS

PUBLIC CLASS CExecListEnumerator IMPLEMENTS IEnumerator

    PUBLIC:

        DECLARE EXTERN CONSTRUCTOR (BYVAL list AS CExecList PTR)

        DECLARE EXTERN VIRTUAL FUNCTION MoveNext() AS BOOLEAN
        DECLARE EXTERN VIRTUAL PROPERTY Current AS ANY
        DECLARE EXTERN VIRTUAL SUB      Reset()

    PRIVATE:

        AS CExecList PTR  _list
        AS CExecNode PTR  _currentElement

END CLASS


