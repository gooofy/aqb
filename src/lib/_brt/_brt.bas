'
' _brt: BASIC runtime, minimal core needed to support the compiler's generated code
'

OPTION EXPLICIT
OPTION PRIVATE

' --------------------------------------------------------------------------------------------------------
' --
' -- ERROR codes for _brt  0xx
' --
' --------------------------------------------------------------------------------------------------------

PUBLIC EXTERN ERR AS INTEGER

PUBLIC CONST AS INTEGER ERR_OUT_OF_DATA             =   4
PUBLIC CONST AS INTEGER ERR_ILLEGAL_FUNCTION_CALL   =   5
PUBLIC CONST AS INTEGER ERR_OUT_OF_MEMORY           =   7
PUBLIC CONST AS INTEGER ERR_SUBSCRIPT_OUT_OF_RANGE  =   9
PUBLIC CONST AS INTEGER ERR_INCOMPATIBLE_ARRAY      =  10

PUBLIC CONST AS INTEGER ERR_BAD_FILE_NUMBER         =  52
PUBLIC CONST AS INTEGER ERR_BAD_FILE_MODE           =  54
PUBLIC CONST AS INTEGER ERR_IO_ERROR                =  57
PUBLIC CONST AS INTEGER ERR_BAD_FILE_NAME           =  64

' --------------------------------------------------------------------------------------------------------
' --
' -- OOP support, Garbage Collector Interface (gc)
' --
' --------------------------------------------------------------------------------------------------------

' forward declare STRING right away as it is needed in ToStrin()
PUBLIC TYPE STRING AS CString PTR

' every class inherits from CObject which is declared here
PUBLIC CLASS CObject

    PRIVATE:

        ' GC support *DO NOT TOUCH!*
        AS ANY PTR __gc_next, __gc_prev
        AS ULONG   __gc_size
        AS UBYTE   __gc_color

    PUBLIC:

        DECLARE EXTERN VIRTUAL Sub      Finalize    ()

        DECLARE EXTERN VIRTUAL FUNCTION ToString    ()                         AS STRING
        DECLARE EXTERN VIRTUAL FUNCTION Equals      (BYVAL obj AS CObject PTR) AS BOOLEAN
        DECLARE EXTERN VIRTUAL FUNCTION GetHashCode ()                         AS ULONG

END CLASS

' garbage collector interface

PUBLIC DECLARE EXTERN SUB      GC_RUN
PUBLIC DECLARE EXTERN SUB      GC_REGISTER   (BYVAL p AS CObject PTR)
PUBLIC DECLARE EXTERN FUNCTION GC_ALLOCATE   (BYVAL size AS ULONG, BYVAL flags AS ULONG=0) AS ANY PTR
PUBLIC DECLARE EXTERN SUB      GC_MARK_BLACK (BYVAL p AS CObject PTR) ' called gc_scan methods
PUBLIC DECLARE EXTERN FUNCTION GC_REACHABLE  (BYVAL p AS CObject PTR) AS BOOLEAN ' test/debug purposes
' FIXME: PUBLIC DECLARE EXTERN SUB      GC_OPTIONS    (BYVAL heap_limit AS ULONG=16*1024, BYVAL alloc_limit AS ULONG=128)

' OOP support: common interfaces

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

        ' remove all items from the list
        DECLARE VIRTUAL SUB RemoveAll

END INTERFACE

PUBLIC INTERFACE ICloneable

    PUBLIC:

        DECLARE VIRTUAL FUNCTION Clone () AS CObject PTR

END INTERFACE

PUBLIC INTERFACE IComparable

    PUBLIC:

        DECLARE VIRTUAL FUNCTION CompareTo (BYVAL obj AS CObject PTR) AS INTEGER

END INTERFACE

' --------------------------------------------------------------------------------------------------------
' --
' -- string support
' --
' --------------------------------------------------------------------------------------------------------

PUBLIC CLASS CString IMPLEMENTS IComparable, ICloneable

    PUBLIC:

        DECLARE EXTERN SHARED FUNCTION Create (BYVAL str AS UBYTE PTR, BYVAL owned AS BOOLEAN) AS CString PTR

        DECLARE EXTERN CONSTRUCTOR (BYVAL str AS UBYTE PTR, BYVAL owned AS BOOLEAN)

        DECLARE EXTERN FUNCTION GetCharAt (BYVAL idx AS ULONG) AS UBYTE

        DECLARE EXTERN PROPERTY Length AS ULONG
        DECLARE EXTERN PROPERTY Str    AS UBYTE PTR

        ' CObject overrides
        DECLARE EXTERN VIRTUAL SUB Finalize ()

        ' ICloneable interface:
        DECLARE EXTERN VIRTUAL FUNCTION Clone () AS CObject PTR

        ' IComparable interface:
        DECLARE VIRTUAL FUNCTION CompareTo (BYVAL obj AS CObject PTR) AS INTEGER

        ' CObject overrides:
        DECLARE EXTERN VIRTUAL FUNCTION ToString    ()                         AS STRING
        DECLARE EXTERN VIRTUAL FUNCTION Equals      (BYVAL obj AS CObject PTR) AS BOOLEAN
        DECLARE EXTERN VIRTUAL FUNCTION GetHashCode ()                         AS ULONG

    PRIVATE:
        AS UBYTE PTR _str
        AS ULONG     _len
        AS ULONG     _hashcode
        AS BOOLEAN   _owned   ' TRUE -> will DEALLOCATE() _str in finalizer

END CLASS

' STR$ support

' FIXME PUBLIC DECLARE EXTERN FUNCTION _S1TOA   (BYVAL b AS BYTE    ) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION _S2TOA   (BYVAL i AS INTEGER ) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION _S4TOA   (BYVAL l AS LONG    ) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION _U1TOA   (BYVAL b AS UBYTE   ) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION _U2TOA   (BYVAL i AS UINTEGER) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION _U4TOA   (BYVAL l AS ULONG   ) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION _FTOA    (BYVAL f AS SINGLE  ) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION _BOOLTOA (BYVAL b AS BOOLEAN ) AS STRING

' HEX$, OCT$, BIN$ support

' FIXME PUBLIC DECLARE EXTERN FUNCTION HEX$ (BYVAL l AS ULONG    ) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION OCT$ (BYVAL l AS ULONG    ) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION BIN$ (BYVAL l AS ULONG    ) AS STRING

' VAL* support

' FIXME 'PUBLIC DECLARE EXTERN FUNCTION _STR2I4_ (BYVAL s AS STRING, BYVAL l AS INTEGER, BYVAL b AS INTEGER) AS LONG
' FIXME 'PUBLIC DECLARE EXTERN FUNCTION _STR2F_  (BYVAL s AS STRING, BYVAL l AS INTEGER, BYVAL b AS INTEGER) AS SINGLE

' FIXME PUBLIC DECLARE EXTERN FUNCTION VAL      (BYVAL s AS STRING) AS SINGLE
' FIXME PUBLIC DECLARE EXTERN FUNCTION VALINT   (BYVAL s AS STRING) AS INTEGER
' FIXME PUBLIC DECLARE EXTERN FUNCTION VALUINT  (BYVAL s AS STRING) AS UINTEGER
' FIXME PUBLIC DECLARE EXTERN FUNCTION VALLNG   (BYVAL s AS STRING) AS LONG
' FIXME PUBLIC DECLARE EXTERN FUNCTION VALULNG  (BYVAL s AS STRING) AS ULONG

' BASIC string functions

' FIXME PUBLIC DECLARE EXTERN FUNCTION LEN     (BYVAL s AS STRING)  AS LONG
' FIXME PUBLIC DECLARE EXTERN FUNCTION CHR$    (BYVAL i AS INTEGER) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION ASC     (BYVAL s AS STRING)  AS INTEGER
' FIXME PUBLIC DECLARE EXTERN FUNCTION MID$    (BYVAL s AS STRING, BYVAL n AS INTEGER, BYVAL m AS INTEGER=-1) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION UCASE$  (BYVAL s AS STRING) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION LCASE$  (BYVAL s AS STRING) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION LEFT$   (BYVAL s AS STRING, BYVAL n AS INTEGER) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION RIGHT$  (BYVAL s AS STRING, BYVAL n AS INTEGER) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION INSTR   (BYVAL n AS INTEGER=1, BYVAL x AS STRING, BYVAL y AS STRING) AS INTEGER
' FIXME PUBLIC DECLARE EXTERN FUNCTION SPACE$  (BYVAL n AS INTEGER) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION SPC     (BYVAL n AS INTEGER) AS STRING
' FIXME PUBLIC DECLARE EXTERN FUNCTION STRING$ (BYVAL n AS INTEGER, BYVAL s AS STRING) AS STRING

' utility function, intended for use by the compiler and C runtime
PUBLIC DECLARE EXTERN FUNCTION _CREATE_CSTRING (BYVAL str AS UBYTE PTR, BYVAL owned AS BOOLEAN) AS CString PTR

' --------------------------------------------------------------------------------------------------------
' --
' -- dynamic array support
' --
' --------------------------------------------------------------------------------------------------------

PUBLIC TYPE CArrayBounds
    PUBLIC:
        AS LONG lbound, ubound, numElements
END TYPE

PUBLIC CLASS CArray IMPLEMENTS IList, ICloneable

    PUBLIC:
        DECLARE EXTERN CONSTRUCTOR     (BYVAL elementSize AS LONG=4)

        ' compiler api:
        DECLARE EXTERN SUB      REDIM  (BYVAL numDims AS UINTEGER, BYVAL preserve AS BOOLEAN, ...)
        DECLARE EXTERN FUNCTION IDXPTR (BYVAL dimCnt AS UINTEGER, ...) AS ANY PTR
        DECLARE EXTERN FUNCTION LBOUND (BYVAL d AS INTEGER) AS LONG
        DECLARE EXTERN FUNCTION UBOUND (BYVAL d AS INTEGER) AS LONG
        DECLARE EXTERN SUB      COPY   (BYREF darray AS CArray)
        DECLARE EXTERN SUB      ERASE  ()

        ' IList interface:
        DECLARE EXTERN VIRTUAL PROPERTY Count AS LONG
        DECLARE EXTERN VIRTUAL PROPERTY Capacity AS LONG
        DECLARE EXTERN VIRTUAL PROPERTY Capacity (BYVAL c AS LONG)
        DECLARE EXTERN VIRTUAL FUNCTION GetAt (BYVAL index AS LONG) AS ANY
        DECLARE EXTERN VIRTUAL SUB      SetAt (BYVAL index AS LONG, BYVAL obj AS ANY)
        DECLARE EXTERN VIRTUAL FUNCTION GetEnumerator () AS IEnumerator PTR
        DECLARE EXTERN VIRTUAL FUNCTION Add (BYVAL obj AS ANY) AS LONG
        DECLARE EXTERN VIRTUAL FUNCTION Contains (BYVAL value AS ANY) AS BOOLEAN
        DECLARE EXTERN VIRTUAL PROPERTY IsReadOnly AS BOOLEAN
        DECLARE EXTERN VIRTUAL PROPERTY IsFixedSize AS BOOLEAN
        DECLARE EXTERN VIRTUAL FUNCTION IndexOf (BYVAL value AS ANY, BYVAL startIndex AS LONG=0, BYVAL count AS LONG=-1) AS LONG
        DECLARE EXTERN VIRTUAL SUB      Insert (BYVAL index AS LONG, BYVAL value AS ANY)
        DECLARE EXTERN VIRTUAL SUB      Remove (BYVAL value AS ANY)
        DECLARE EXTERN VIRTUAL SUB      RemoveAt (BYVAL index AS LONG)
        DECLARE EXTERN VIRTUAL SUB      RemoveAll

        ' ICloneable interface:
        DECLARE EXTERN VIRTUAL FUNCTION Clone () AS CObject PTR

    PRIVATE:

        AS ANY PTR          _data
        AS UINTEGER         _numDims
        AS LONG             _elementSize
        AS LONG             _dataSize
        AS CArrayBounds PTR _bounds

END CLASS

PUBLIC CLASS CArrayEnumerator IMPLEMENTS IEnumerator

    PUBLIC:

        DECLARE EXTERN CONSTRUCTOR (BYVAL list AS CArray PTR)

        DECLARE EXTERN VIRTUAL FUNCTION MoveNext() AS BOOLEAN
        DECLARE EXTERN VIRTUAL PROPERTY Current AS ANY
        DECLARE EXTERN VIRTUAL SUB      Reset()

    PRIVATE:

        AS CArray PTR _array
        AS LONG       _index
        AS ANY        _currentElement

END CLASS

' --------------------------------------------------------------------------------------------------------
' --
' -- TRACE support
' --
' --------------------------------------------------------------------------------------------------------

PUBLIC DECLARE EXTERN SUB _DEBUG_PUTS    (BYVAL s AS STRING)
PUBLIC DECLARE EXTERN SUB _DEBUG_PUTS1   (BYVAL s AS BYTE)
PUBLIC DECLARE EXTERN SUB _DEBUG_PUTS2   (BYVAL s AS INTEGER)
PUBLIC DECLARE EXTERN SUB _DEBUG_PUTS4   (BYVAL s AS LONG)
PUBLIC DECLARE EXTERN SUB _DEBUG_PUTU1   (BYVAL s AS UBYTE)
PUBLIC DECLARE EXTERN SUB _DEBUG_PUTU2   (BYVAL s AS UINTEGER)
PUBLIC DECLARE EXTERN SUB _DEBUG_PUTU4   (BYVAL s AS ULONG)
PUBLIC DECLARE EXTERN SUB _DEBUG_PUTF    (BYVAL f AS SINGLE)
PUBLIC DECLARE EXTERN SUB _DEBUG_PUTBOOl (BYVAL b AS BOOLEAN)
PUBLIC DECLARE EXTERN SUB _DEBUG_PUTTAB
PUBLIC DECLARE EXTERN SUB _DEBUG_PUTNL
PUBLIC DECLARE EXTERN SUB _DEBUG_CLS
PUBLIC DECLARE EXTERN SUB _DEBUG_BREAK

' AmigaOS library bases

PUBLIC EXTERN SysBase AS ANY PTR
PUBLIC EXTERN DOSBase AS ANY PTR
PUBLIC EXTERN MathBase AS ANY PTR
PUBLIC EXTERN MathTransBase AS ANY PTR

' math support

PUBLIC CONST AS INTEGER INTEGER_MIN = -32768
PUBLIC CONST AS INTEGER INTEGER_MAX =  32767

PUBLIC DECLARE EXTERN FUNCTION FIX   (BYVAL f AS SINGLE) AS INTEGER
PUBLIC DECLARE EXTERN FUNCTION INT   (BYVAL f AS SINGLE) AS INTEGER
PUBLIC DECLARE EXTERN FUNCTION CINT  (BYVAL f AS SINGLE) AS INTEGER
PUBLIC DECLARE EXTERN FUNCTION CLNG  (BYVAL f AS SINGLE) AS LONG
PUBLIC DECLARE EXTERN FUNCTION SGN   (BYVAL n AS SINGLE) AS INTEGER

PUBLIC CONST AS SINGLE PI = 3.14159

PUBLIC DECLARE EXTERN FUNCTION ATN  (BYVAL f AS SINGLE) AS SINGLE LIB  -30 MathTransBase (d0)
PUBLIC DECLARE EXTERN FUNCTION SIN  (BYVAL f AS SINGLE) AS SINGLE LIB  -36 MathTransBase (d0)
PUBLIC DECLARE EXTERN FUNCTION COS  (BYVAL f AS SINGLE) AS SINGLE LIB  -42 MathTransBase (d0)
PUBLIC DECLARE EXTERN FUNCTION TAN  (BYVAL f AS SINGLE) AS SINGLE LIB  -48 MathTransBase (d0)
PUBLIC DECLARE EXTERN FUNCTION EXP  (BYVAL f AS SINGLE) AS SINGLE LIB  -78 MathTransBase (d0)
PUBLIC DECLARE EXTERN FUNCTION LOG  (BYVAL f AS SINGLE) AS SINGLE LIB  -84 MathTransBase (d0)
PUBLIC DECLARE EXTERN FUNCTION SQR  (BYVAL f AS SINGLE) AS SINGLE LIB  -96 MathTransBase (d0)
PUBLIC DECLARE EXTERN FUNCTION ASIN (BYVAL f AS SINGLE) AS SINGLE LIB -114 MathTransBase (d0)
PUBLIC DECLARE EXTERN FUNCTION ACOS (BYVAL f AS SINGLE) AS SINGLE LIB -120 MathTransBase (d0)

PUBLIC DECLARE EXTERN FUNCTION ABS  (BYVAL f AS SINGLE) AS SINGLE LIB  -54 MathBase (d0)

' --------------------------------------------------------------------------------------------------------
' --
' -- dynamic memory
' --
' --------------------------------------------------------------------------------------------------------

PUBLIC DECLARE EXTERN FUNCTION ALLOCATE   (BYVAL size AS ULONG, BYVAL flags AS ULONG=0) AS ANY PTR
PUBLIC DECLARE EXTERN SUB      DEALLOCATE (BYVAL p AS ANY PTR)
PUBLIC DECLARE EXTERN SUB      _MEMSET    (BYVAL dst AS BYTE PTR, BYVAL c AS BYTE, BYVAL n AS ULONG)
PUBLIC DECLARE EXTERN FUNCTION FRE        (BYVAL x AS INTEGER) AS ULONG

PUBLIC DECLARE EXTERN SUB      POKE       (BYVAL adr AS ULONG, BYVAL b AS UBYTE)
PUBLIC DECLARE EXTERN SUB      POKEW      (BYVAL adr AS ULONG, BYVAL w AS UINTEGER)
PUBLIC DECLARE EXTERN SUB      POKEL      (BYVAL adr AS ULONG, BYVAL l AS ULONG)

PUBLIC DECLARE EXTERN FUNCTION PEEK       (BYVAL adr AS ULONG) AS UBYTE
PUBLIC DECLARE EXTERN FUNCTION PEEKW      (BYVAL adr AS ULONG) AS UINTEGER
PUBLIC DECLARE EXTERN FUNCTION PEEKL      (BYVAL adr AS ULONG) AS ULONG

PUBLIC DECLARE EXTERN FUNCTION CRC32      (BYVAL p AS ANY PTR, BYVAL l AS ULONG)

' random numbers

PUBLIC DECLARE EXTERN FUNCTION RND       (BYVAL n AS SINGLE = 1) AS SINGLE
PUBLIC DECLARE EXTERN SUB      RANDOMIZE (BYVAL seed AS SINGLE)
' DATA / READ / RESTORE support

PUBLIC DECLARE EXTERN SUB _AQB_RESTORE (BYVAL p AS ANY PTR)
PUBLIC DECLARE EXTERN SUB _AQB_READ1   (BYVAL v AS ANY PTR)
PUBLIC DECLARE EXTERN SUB _AQB_READ2   (BYVAL v AS ANY PTR)
PUBLIC DECLARE EXTERN SUB _AQB_READ4   (BYVAL v AS ANY PTR)
PUBLIC DECLARE EXTERN SUB _AQB_READSTR (BYVAL v AS ANY PTR)

' PRINT statement support:

PUBLIC DECLARE EXTERN SUB _AIO_PUTS    (BYVAL fno AS UINTEGER, BYVAL s AS STRING  )
PUBLIC DECLARE EXTERN SUB _AIO_PUTS1   (BYVAL fno AS UINTEGER, BYVAL b AS BYTE    )
PUBLIC DECLARE EXTERN SUB _AIO_PUTS2   (BYVAL fno AS UINTEGER, BYVAL i AS INTEGER )
PUBLIC DECLARE EXTERN SUB _AIO_PUTS4   (BYVAL fno AS UINTEGER, BYVAL l AS LONG    )
PUBLIC DECLARE EXTERN SUB _AIO_PUTU1   (BYVAL fno AS UINTEGER, BYVAL u AS UBYTE   )
PUBLIC DECLARE EXTERN SUB _AIO_PUTU2   (BYVAL fno AS UINTEGER, BYVAL u AS UINTEGER)
PUBLIC DECLARE EXTERN SUB _AIO_PUTU4   (BYVAL fno AS UINTEGER, BYVAL u AS ULONG   )
PUBLIC DECLARE EXTERN SUB _AIO_PUTF    (BYVAL fno AS UINTEGER, BYVAL f AS SINGLE  )
PUBLIC DECLARE EXTERN SUB _AIO_PUTBOOL (BYVAL fno AS UINTEGER, BYVAL b AS BOOLEAN )
PUBLIC DECLARE EXTERN SUB _AIO_PUTNL   (BYVAL fno AS UINTEGER)
PUBLIC DECLARE EXTERN SUB _AIO_PUTTAB  (BYVAL fno AS UINTEGER)

' WRITE statement support:

PUBLIC DECLARE EXTERN SUB _AIO_WRITES4   (BYVAL fno AS UINTEGER, BYVAL l AS LONG    )
PUBLIC DECLARE EXTERN SUB _AIO_WRITES2   (BYVAL fno AS UINTEGER, BYVAL i AS INTEGER )
PUBLIC DECLARE EXTERN SUB _AIO_WRITES1   (BYVAL fno AS UINTEGER, BYVAL b AS BYTE    )
PUBLIC DECLARE EXTERN SUB _AIO_WRITEU4   (BYVAL fno AS UINTEGER, BYVAL u AS ULONG   )
PUBLIC DECLARE EXTERN SUB _AIO_WRITEU2   (BYVAL fno AS UINTEGER, BYVAL u AS UINTEGER)
PUBLIC DECLARE EXTERN SUB _AIO_WRITEU1   (BYVAL fno AS UINTEGER, BYVAL u AS UBYTE   )
PUBLIC DECLARE EXTERN SUB _AIO_WRITEF    (BYVAL fno AS UINTEGER, BYVAL f AS SINGLE  )
PUBLIC DECLARE EXTERN SUB _AIO_WRITEBOOL (BYVAL fno AS UINTEGER, BYVAL b AS BOOLEAN )
PUBLIC DECLARE EXTERN SUB _AIO_WRITES    (BYVAL fno AS UINTEGER, BYVAL s AS STRING  )
PUBLIC DECLARE EXTERN SUB _AIO_WRITECOMMA(BYVAL fno AS UINTEGER                     )

' [ LINE ] INPUT support:

PUBLIC DECLARE EXTERN SUB _AIO_LINE_INPUT             (BYVAL prompt AS STRING, BYREF s AS STRING, BYVAL do_nl AS BOOLEAN)
PUBLIC DECLARE EXTERN SUB _AIO_CONSOLE_INPUT          (BYVAL qm AS BOOLEAN, BYVAL prompt AS STRING, BYVAL do_nl AS BOOLEAN)
' FIXME PUBLIC DECLARE EXTERN SUB _AIO_INPUTS1                (BYVAL fno AS UINTEGER, BYREF v AS BYTE    )
' FIXME PUBLIC DECLARE EXTERN SUB _AIO_INPUTU1                (BYVAL fno AS UINTEGER, BYREF v AS UBYTE   )
' FIXME PUBLIC DECLARE EXTERN SUB _AIO_INPUTS2                (BYVAL fno AS UINTEGER, BYREF v AS INTEGER )
' FIXME PUBLIC DECLARE EXTERN SUB _AIO_INPUTU2                (BYVAL fno AS UINTEGER, BYREF v AS UINTEGER)
' FIXME PUBLIC DECLARE EXTERN SUB _AIO_INPUTS4                (BYVAL fno AS UINTEGER, BYREF v AS LONG    )
' FIXME PUBLIC DECLARE EXTERN SUB _AIO_INPUTU4                (BYVAL fno AS UINTEGER, BYREF v AS ULONG   )
' FIXME PUBLIC DECLARE EXTERN SUB _AIO_INPUTF                 (BYVAL fno AS UINTEGER, BYREF v AS SINGLE  )
' FIXME PUBLIC DECLARE EXTERN SUB _AIO_INPUTS                 (BYVAL fno AS UINTEGER, BYREF v AS STRING  )
PUBLIC DECLARE EXTERN SUB _AIO_SET_DOS_CURSOR_VISIBLE (BYVAL visible AS BOOLEAN)

PUBLIC DECLARE EXTERN SUB CLS
PUBLIC DECLARE EXTERN SUB LOCATE                      (BYVAL l AS INTEGER=-1, BYVAL c AS INTEGER=-1)

' file i/o

PUBLIC CONST AS UINTEGER FILE_MODE_RANDOM      = 0
PUBLIC CONST AS UINTEGER FILE_MODE_INPUT       = 1
PUBLIC CONST AS UINTEGER FILE_MODE_OUTPUT      = 2
PUBLIC CONST AS UINTEGER FILE_MODE_APPEND      = 3
PUBLIC CONST AS UINTEGER FILE_MODE_BINARY      = 4

PUBLIC CONST AS UINTEGER FILE_ACCESS_READ      = 0
PUBLIC CONST AS UINTEGER FILE_ACCESS_WRITE     = 1
PUBLIC CONST AS UINTEGER FILE_ACCESS_READWRITE = 2

PUBLIC DECLARE EXTERN SUB _AIO_OPEN  (BYVAL fname AS STRING, BYVAL mode AS UINTEGER, BYVAL access AS UINTEGER, BYVAL fno AS UINTEGER, BYVAL recordlen AS UINTEGER)
PUBLIC DECLARE EXTERN SUB _AIO_CLOSE (BYVAL fno AS UINTEGER)

PUBLIC DECLARE EXTERN FUNCTION EOF (BYVAL fno AS UINTEGER) AS BOOLEAN

' misc

PUBLIC DECLARE EXTERN SUB      _AQB_ASSERT   (BYVAL b AS BOOLEAN, BYVAL s AS STRING)
PUBLIC DECLARE EXTERN SUB      ERROR         (BYVAL i AS INTEGER)
PUBLIC DECLARE EXTERN SUB      RESUME NEXT
PUBLIC DECLARE EXTERN SUB      ON ERROR CALL (BYVAL p AS SUB)
PUBLIC DECLARE EXTERN SUB      ON EXIT CALL  (BYVAL p AS SUB)
PUBLIC DECLARE EXTERN SUB      ON BREAK CALL (BYVAL p AS SUB)

PUBLIC DECLARE EXTERN SUB      SYSTEM

PUBLIC DECLARE EXTERN FUNCTION TIMER         () AS SINGLE
PUBLIC DECLARE EXTERN FUNCTION TIME$         ()
PUBLIC DECLARE EXTERN FUNCTION DATE$         ()
PUBLIC DECLARE EXTERN SUB      SLEEP FOR     (BYVAL s AS SINGLE)

PUBLIC EXTERN g_stdout AS ANY PTR

