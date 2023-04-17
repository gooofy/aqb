
OPTION EXPLICIT
OPTION PRIVATE

' collections

PUBLIC INTERFACE IEnumerator

    PUBLIC:

        DECLARE VIRTUAL FUNCTION MoveNext() AS BOOLEAN
        DECLARE VIRTUAL PROPERTY Current AS ANY
        DECLARE VIRTUAL SUB      Reset()

END INTERFACE

PUBLIC INTERFACE IEnumerable

    PUBLIC:

        DECLARE VIRTUAL FUNCTION GetEnumerator () AS IEnumerator PTR

END INTERFACE

PUBLIC INTERFACE ICollection IMPLEMENTS IEnumerable

    PUBLIC:

        DECLARE VIRTUAL PROPERTY Count AS LONG

END INTERFACE

PUBLIC INTERFACE IList IMPLEMENTS ICollection

    PUBLIC:

        DECLARE VIRTUAL FUNCTION GetAt (BYVAL index AS LONG) AS ANY
        DECLARE VIRTUAL SUB      SetAt (BYVAL index AS LONG, BYVAL obj AS ANY)

        ' add an item to the list, exact position is implementation dependant
        ' return value is the position the new element was inserted in
        DECLARE VIRTUAL FUNCTION Add (BYVAL obj AS ANY) AS LONG

        ' return whether the list contains a particular item
        DECLARE VIRTUAL FUNCTION Contains (BYVAL value AS ANY) AS BOOLEAN

        ' remove all items from the list
        DECLARE VIRTUAL SUB Clear

        DECLARE VIRTUAL PROPERTY IsReadOnly AS BOOLEAN

        DECLARE VIRTUAL PROPERTY IsFixedSize AS BOOLEAN

        ' return the index of a particular item, if it is in the list
        ' return -1 if the item isn't in the list
        DECLARE VIRTUAL FUNCTION IndexOf (BYVAL value AS ANY, BYVAL startIndex AS LONG=0, BYVAL count AS LONG=-1) AS LONG

        ' insert value into the list at position index.
        ' index must be non-negative and less than or equal to the
        ' number of elements in the list.  If index equals the number
        ' of items in the list, then value is appended to the end
        DECLARE VIRTUAL SUB Insert (BYVAL index AS LONG, BYVAL value AS ANY)

        ' remove an item from the list
        DECLARE VIRTUAL SUB Remove (BYVAL value AS ANY)

        ' remove the item at position index
        DECLARE VIRTUAL SUB RemoveAt (BYVAL index AS LONG)

END INTERFACE

PUBLIC INTERFACE ICloneable

    PUBLIC:

        DECLARE VIRTUAL FUNCTION Clone () AS CObject PTR

END INTERFACE

PUBLIC INTERFACE IComparable

    PUBLIC:

        DECLARE VIRTUAL FUNCTION CompareTo (BYVAL obj AS CObject PTR) AS INTEGER

END INTERFACE

PUBLIC CLASS ArrayList IMPLEMENTS IList, ICloneable

    PUBLIC:

        DECLARE EXTERN CONSTRUCTOR (BYVAL capacity AS LONG = 0)

        DECLARE EXTERN VIRTUAL PROPERTY Count AS LONG

        DECLARE EXTERN VIRTUAL PROPERTY Capacity AS LONG
        DECLARE EXTERN VIRTUAL PROPERTY Capacity (BYVAL c AS LONG)

        DECLARE EXTERN VIRTUAL FUNCTION GetAt (BYVAL index AS LONG) AS ANY
        DECLARE EXTERN VIRTUAL SUB      SetAt (BYVAL index AS LONG, BYVAL obj AS ANY)

        DECLARE EXTERN VIRTUAL FUNCTION GetEnumerator () AS IEnumerator PTR

        DECLARE EXTERN VIRTUAL FUNCTION Clone () AS CObject PTR

        DECLARE EXTERN VIRTUAL FUNCTION Add (BYVAL obj AS ANY) AS LONG

        DECLARE EXTERN VIRTUAL FUNCTION Contains (BYVAL value AS ANY) AS BOOLEAN

        DECLARE EXTERN VIRTUAL SUB Clear

        DECLARE EXTERN VIRTUAL PROPERTY IsReadOnly AS BOOLEAN

        DECLARE EXTERN VIRTUAL PROPERTY IsFixedSize AS BOOLEAN

        DECLARE EXTERN VIRTUAL FUNCTION IndexOf (BYVAL value AS ANY, BYVAL startIndex AS LONG=0, BYVAL count AS LONG=-1) AS LONG

        DECLARE EXTERN VIRTUAL SUB Insert (BYVAL index AS LONG, BYVAL value AS ANY)

        DECLARE EXTERN VIRTUAL SUB Remove (BYVAL value AS ANY)

        DECLARE EXTERN VIRTUAL SUB RemoveAt (BYVAL index AS LONG)

    PRIVATE:

		AS ANY PTR    _items
        AS LONG       _size
        AS LONG       _capacity

END CLASS

'PUBLIC CLASS CString IMPLEMENTS IComparable, IEnumerable, ICloneable
'
'    PUBLIC:
'
'        DECLARE EXTERN CONSTRUCTOR (BYVAL str AS STRING)
'
'END CLASS


