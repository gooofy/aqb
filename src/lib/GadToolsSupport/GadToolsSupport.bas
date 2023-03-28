OPTION EXPLICIT

IMPORT OSGadTools

CONST AE_GTG_CREATE   AS INTEGER = 400
CONST AE_GTG_MODIFY   AS INTEGER = 401
CONST AE_GTG_DEPLOY   AS INTEGER = 402
CONST AE_GTG_SELECTED AS INTEGER = 403
CONST AE_GTG_CALLBACK AS INTEGER = 404
CONST AE_GTG_BUFFER   AS INTEGER = 405
CONST AE_GTG_NUM      AS INTEGER = 406

TYPE GTGADGET

    PUBLIC:

        DECLARE CONSTRUCTOR (BYVAL label AS STRING, _
                             _COORD2(BYVAL s1 AS BOOLEAN, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                             BYVAL user_data AS VOID PTR=NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE PROPERTY x1 AS INTEGER
        DECLARE PROPERTY x1 (BYVAL x AS INTEGER)

        DECLARE PROPERTY y1 AS INTEGER
        DECLARE PROPERTY y1 (BYVAL y AS INTEGER)

        DECLARE PROPERTY x2 AS INTEGER
        DECLARE PROPERTY x2 (BYVAL x AS INTEGER)

        DECLARE PROPERTY y2 AS INTEGER
        DECLARE PROPERTY y2 (BYVAL y AS INTEGER)

        DECLARE PROPERTY text AS STRING
        DECLARE PROPERTY text (BYVAL txt AS STRING)

        DECLARE PROPERTY id AS INTEGER
        DECLARE PROPERTY id (BYVAL id AS INTEGER)

        DECLARE PROPERTY flags AS ULONG
        DECLARE PROPERTY flags (BYVAL f AS ULONG)

        DECLARE PROPERTY deployed AS BOOLEAN

        AS SUB (BYVAL GTGADGET PTR, BYVAL UINTEGER) gadgetup_cb
        AS SUB (BYVAL GTGADGET PTR, BYVAL UINTEGER) gadgetdown_cb
        AS SUB (BYVAL GTGADGET PTR, BYVAL UINTEGER) gadgetmove_cb

        AS VOID PTR user_data
        AS ULONG    underscore

    PRIVATE:

        AS GTGADGET PTR   prev, next

        AS NewGadget      ng

        AS Gadget PTR     gad
        AS Window PTR     win

        AS VOID PTR       deploy_cb


END TYPE

TYPE GTBUTTON EXTENDS GTGADGET

    PUBLIC:
        DECLARE CONSTRUCTOR (BYVAL label AS STRING, _
                             _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                             BYVAL user_data AS VOID PTR=NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE PROPERTY disabled AS BOOLEAN
        DECLARE PROPERTY disabled (BYVAL b AS BOOLEAN)

        DECLARE PROPERTY immediate AS BOOLEAN
        DECLARE PROPERTY immediate (BYVAL b AS BOOLEAN)

    PRIVATE:
        AS BOOLEAN  _disabled
        AS BOOLEAN  _immediate

END TYPE

TYPE GTCHECKBOX EXTENDS GTGADGET

    PUBLIC:
        DECLARE CONSTRUCTOR (BYVAL label AS STRING, _
                             _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                             BYVAL user_data AS VOID PTR=NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE PROPERTY disabled AS BOOLEAN
        DECLARE PROPERTY disabled (BYVAL b AS BOOLEAN)

        DECLARE PROPERTY checked AS BOOLEAN
        DECLARE PROPERTY checked (BYVAL b AS BOOLEAN)

        DECLARE PROPERTY scaled AS BOOLEAN
        DECLARE PROPERTY scaled (BYVAL b AS BOOLEAN)

    PRIVATE:
        AS BOOLEAN  _disabled
        AS BOOLEAN  _checked
        AS BOOLEAN  _scaled

END TYPE

TYPE GTSLIDER EXTENDS GTGADGET

    PUBLIC:

        DECLARE CONSTRUCTOR (BYVAL label AS STRING, BYVAL min AS INTEGER, BYVAL max AS INTEGER, BYVAL level AS INTEGER, BYVAL freedom AS ULONG, _
                             _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                             BYVAL user_data AS VOID PTR=NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE PROPERTY disabled AS BOOLEAN
        DECLARE PROPERTY disabled (BYVAL b AS BOOLEAN)

        DECLARE PROPERTY min AS INTEGER
        DECLARE PROPERTY min (BYVAL i AS INTEGER)

        DECLARE PROPERTY max AS INTEGER
        DECLARE PROPERTY max (BYVAL i AS INTEGER)

        DECLARE PROPERTY level AS INTEGER
        DECLARE PROPERTY level (BYVAL i AS INTEGER)

        DECLARE PROPERTY maxLevelLen AS INTEGER
        DECLARE PROPERTY maxLevelLen (BYVAL i AS INTEGER)

        DECLARE PROPERTY levelFormat AS STRING
        DECLARE PROPERTY levelFormat (BYVAL s AS STRING)

        DECLARE PROPERTY levelPlace AS ULONG
        DECLARE PROPERTY levelPlace (BYVAL u AS ULONG)

        DECLARE PROPERTY immediate AS BOOLEAN
        DECLARE PROPERTY immediate (BYVAL b AS BOOLEAN)

        DECLARE PROPERTY relVerify AS BOOLEAN
        DECLARE PROPERTY relVerify (BYVAL b AS BOOLEAN)

        DECLARE PROPERTY freedom AS ULONG
        DECLARE PROPERTY freedom (BYVAL u AS ULONG)

    PRIVATE:
        AS BOOLEAN  _disabled
        AS INTEGER  _min, _max, _level
        AS ULONG    _freedom
        AS INTEGER  _maxLevelLen
        AS STRING   _levelFormat
        AS ULONG    _levelPlace
        AS BOOLEAN  _immediate, _relVerify

END TYPE

TYPE GTTEXT EXTENDS GTGADGET

    PUBLIC:
        DECLARE CONSTRUCTOR ( BYVAL label AS STRING, BYVAL text AS STRING, _
                              _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                              BYVAL user_data AS VOID PTR=NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE PROPERTY text AS STRING
        DECLARE PROPERTY text (BYVAL value AS STRING)

        DECLARE PROPERTY copyText AS BOOLEAN
        DECLARE PROPERTY copyText (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY border AS BOOLEAN
        DECLARE PROPERTY border (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY frontPen AS UBYTE
        DECLARE PROPERTY frontPen (BYVAL value AS UBYTE)

        DECLARE PROPERTY backPen AS UBYTE
        DECLARE PROPERTY backPen (BYVAL value AS UBYTE)

        DECLARE PROPERTY justification AS UBYTE
        DECLARE PROPERTY justification (BYVAL value AS UBYTE)

        DECLARE PROPERTY clipped AS BOOLEAN
        DECLARE PROPERTY clipped (BYVAL value AS BOOLEAN)

    PRIVATE:

        AS STRING       _text
        AS BOOLEAN      _copyText
        AS BOOLEAN      _border
        AS UBYTE        _frontPen
        AS UBYTE        _backPen
        AS UBYTE        _justification
        AS BOOLEAN      _clipped

END TYPE

TYPE GTSCROLLER EXTENDS GTGADGET

    PUBLIC:
        DECLARE CONSTRUCTOR ( BYVAL label AS STRING, BYVAL top AS INTEGER, BYVAL total AS INTEGER, BYVAL visible AS INTEGER, BYVAL freedom AS ULONG,  _
                              _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                              BYVAL user_data AS VOID PTR=NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE PROPERTY disabled AS BOOLEAN
        DECLARE PROPERTY disabled (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY relVerify AS BOOLEAN
        DECLARE PROPERTY relVerify (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY immediate AS BOOLEAN
        DECLARE PROPERTY immediate (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY top AS INTEGER
        DECLARE PROPERTY top (BYVAL value AS INTEGER)

        DECLARE PROPERTY total AS INTEGER
        DECLARE PROPERTY total (BYVAL value AS INTEGER)

        DECLARE PROPERTY visible AS BOOLEAN
        DECLARE PROPERTY visible (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY arrows AS UINTEGER
        DECLARE PROPERTY arrows (BYVAL value AS UINTEGER)

        DECLARE PROPERTY freedom AS ULONG
        DECLARE PROPERTY freedom (BYVAL value AS ULONG)

    PRIVATE:

        AS BOOLEAN      _disabled
        AS BOOLEAN      _relVerify
        AS BOOLEAN      _immediate
        AS INTEGER      _top
        AS INTEGER      _total
        AS BOOLEAN      _visible
        AS UINTEGER     _arrows
        AS ULONG        _freedom

END TYPE

TYPE GTSTRING EXTENDS GTGADGET

    PUBLIC:
        DECLARE CONSTRUCTOR ( BYVAL label AS STRING,  _
                              _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                              BYVAL user_data AS VOID PTR=NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE PROPERTY disabled AS BOOLEAN
        DECLARE PROPERTY disabled (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY immediate AS BOOLEAN
        DECLARE PROPERTY immediate (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY tabCycle AS BOOLEAN
        DECLARE PROPERTY tabCycle (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY str AS STRING
        DECLARE PROPERTY str (BYVAL value AS STRING)

        DECLARE PROPERTY maxChars AS UINTEGER
        DECLARE PROPERTY maxChars (BYVAL value AS UINTEGER)

        DECLARE PROPERTY exitHelp AS BOOLEAN
        DECLARE PROPERTY exitHelp (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY justification AS STRING
        DECLARE PROPERTY justification (BYVAL value AS STRING)

        DECLARE PROPERTY replaceMode AS BOOLEAN
        DECLARE PROPERTY replaceMode (BYVAL value AS BOOLEAN)

    PRIVATE:

        AS BOOLEAN      _disabled
        AS BOOLEAN      _immediate
        AS BOOLEAN      _tabCycle
        AS STRING       _str
        AS UINTEGER     _maxChars
        AS BOOLEAN      _exitHelp
        AS STRING       _justification
        AS BOOLEAN      _replaceMode

END TYPE

TYPE GTINTEGER EXTENDS GTGADGET

    PUBLIC:
        DECLARE CONSTRUCTOR ( BYVAL label AS STRING,  _
                              _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                              BYVAL user_data AS VOID PTR=NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE PROPERTY disabled AS BOOLEAN
        DECLARE PROPERTY disabled (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY immediate AS BOOLEAN
        DECLARE PROPERTY immediate (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY tabCycle AS BOOLEAN
        DECLARE PROPERTY tabCycle (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY number AS LONG
        DECLARE PROPERTY number (BYVAL value AS LONG)

        DECLARE PROPERTY maxChars AS UINTEGER
        DECLARE PROPERTY maxChars (BYVAL value AS UINTEGER)

        DECLARE PROPERTY exitHelp AS BOOLEAN
        DECLARE PROPERTY exitHelp (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY justification AS STRING
        DECLARE PROPERTY justification (BYVAL value AS STRING)

        DECLARE PROPERTY replaceMode AS BOOLEAN
        DECLARE PROPERTY replaceMode (BYVAL value AS BOOLEAN)

    PRIVATE:

        AS BOOLEAN      _disabled
        AS BOOLEAN      _immediate
        AS BOOLEAN      _tabCycle
        AS LONG         _number
        AS UINTEGER     _maxChars
        AS BOOLEAN      _exitHelp
        AS STRING       _justification
        AS BOOLEAN      _replaceMode

END TYPE

TYPE GTNUMBER EXTENDS GTGADGET

    PUBLIC:
        DECLARE CONSTRUCTOR ( BYVAL label AS STRING, BYVAL number AS LONG,  _
                              _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                              BYVAL user_data AS VOID PTR=NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE PROPERTY number AS LONG
        DECLARE PROPERTY number (BYVAL value AS LONG)

        DECLARE PROPERTY border AS BOOLEAN
        DECLARE PROPERTY border (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY frontPen AS UBYTE
        DECLARE PROPERTY frontPen (BYVAL value AS UBYTE)

        DECLARE PROPERTY backPen AS UBYTE
        DECLARE PROPERTY backPen (BYVAL value AS UBYTE)

        DECLARE PROPERTY justification AS UBYTE
        DECLARE PROPERTY justification (BYVAL value AS UBYTE)

        DECLARE PROPERTY format AS STRING
        DECLARE PROPERTY format (BYVAL value AS STRING)

        DECLARE PROPERTY maxNumberLen AS ULONG
        DECLARE PROPERTY maxNumberLen (BYVAL value AS ULONG)

        DECLARE PROPERTY clipped AS BOOLEAN
        DECLARE PROPERTY clipped (BYVAL value AS BOOLEAN)

    PRIVATE:

        AS LONG         _number
        AS BOOLEAN      _border
        AS UBYTE        _frontPen
        AS UBYTE        _backPen
        AS UBYTE        _justification
        AS STRING       _format
        AS ULONG        _maxNumberLen
        AS BOOLEAN      _clipped

END TYPE

TYPE GTMX EXTENDS GTGADGET

    PUBLIC:
        DECLARE CONSTRUCTOR ( BYVAL label AS STRING, BYVAL labels AS STRING PTR,  _
                              _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                              BYVAL user_data AS VOID PTR=NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE PROPERTY disabled AS BOOLEAN
        DECLARE PROPERTY disabled (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY labels AS STRING PTR
        DECLARE PROPERTY labels (BYVAL value AS STRING PTR)

        DECLARE PROPERTY active AS UINTEGER
        DECLARE PROPERTY active (BYVAL value AS UINTEGER)

        DECLARE PROPERTY spacing AS UINTEGER
        DECLARE PROPERTY spacing (BYVAL value AS UINTEGER)

        DECLARE PROPERTY scaled AS BOOLEAN
        DECLARE PROPERTY scaled (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY titlePlace AS ULONG
        DECLARE PROPERTY titlePlace (BYVAL value AS ULONG)

    PRIVATE:

        AS BOOLEAN      _disabled
        AS STRING PTR   _labels
        AS UINTEGER     _active
        AS UINTEGER     _spacing
        AS BOOLEAN      _scaled
        AS ULONG        _titlePlace

END TYPE

TYPE GTCYCLE EXTENDS GTGADGET

    PUBLIC:
        DECLARE CONSTRUCTOR ( BYVAL label AS STRING, BYVAL labels AS STRING PTR,  _
                              _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                              BYVAL user_data AS VOID PTR=NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE PROPERTY disabled AS BOOLEAN
        DECLARE PROPERTY disabled (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY labels AS STRING PTR
        DECLARE PROPERTY labels (BYVAL value AS STRING PTR)

        DECLARE PROPERTY active AS UINTEGER
        DECLARE PROPERTY active (BYVAL value AS UINTEGER)

    PRIVATE:

        AS BOOLEAN      _disabled
        AS STRING PTR   _labels
        AS UINTEGER     _active

END TYPE

TYPE GTPALETTE EXTENDS GTGADGET

    PUBLIC:
        DECLARE CONSTRUCTOR ( BYVAL label AS STRING, BYVAL numColors AS UINTEGER,  _
                              _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                              BYVAL user_data AS VOID PTR=NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE PROPERTY disabled AS BOOLEAN
        DECLARE PROPERTY disabled (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY color AS UBYTE
        DECLARE PROPERTY color (BYVAL value AS UBYTE)

        DECLARE PROPERTY colorOffset AS UBYTE
        DECLARE PROPERTY colorOffset (BYVAL value AS UBYTE)

        DECLARE PROPERTY indicatorWidth AS UINTEGER
        DECLARE PROPERTY indicatorWidth (BYVAL value AS UINTEGER)

        DECLARE PROPERTY indicatorHeight AS UINTEGER
        DECLARE PROPERTY indicatorHeight (BYVAL value AS UINTEGER)

        DECLARE PROPERTY colorTable AS UBYTE PTR
        DECLARE PROPERTY colorTable (BYVAL value AS UBYTE PTR)

        DECLARE PROPERTY numColors AS UINTEGER
        DECLARE PROPERTY numColors (BYVAL value AS UINTEGER)

    PRIVATE:

        AS BOOLEAN      _disabled
        AS UBYTE        _color
        AS UBYTE        _colorOffset
        AS UINTEGER     _indicatorWidth
        AS UINTEGER     _indicatorHeight
        AS UBYTE PTR    _colorTable
        AS UINTEGER     _numColors

END TYPE

TYPE GTLISTVIEW EXTENDS GTGADGET

    PUBLIC:
        DECLARE CONSTRUCTOR ( BYVAL label AS STRING, BYVAL labels AS ExecList PTR,  _
                              _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                              BYVAL user_data AS VOID PTR=NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE PROPERTY disabled AS BOOLEAN
        DECLARE PROPERTY disabled (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY makeVisible AS INTEGER
        DECLARE PROPERTY makeVisible (BYVAL value AS INTEGER)

        DECLARE PROPERTY labels AS ExecList PTR
        DECLARE PROPERTY labels (BYVAL value AS ExecList PTR)

        DECLARE PROPERTY readOnly AS BOOLEAN
        DECLARE PROPERTY readOnly (BYVAL value AS BOOLEAN)

        DECLARE PROPERTY scrollWidth AS UINTEGER
        DECLARE PROPERTY scrollWidth (BYVAL value AS UINTEGER)

        DECLARE PROPERTY selected AS UINTEGER
        DECLARE PROPERTY selected (BYVAL value AS UINTEGER)

        DECLARE PROPERTY spacing AS UINTEGER
        DECLARE PROPERTY spacing (BYVAL value AS UINTEGER)

    PRIVATE:

        AS BOOLEAN      _disabled
        AS INTEGER      _makeVisible
        AS ExecList PTR _labels
        AS BOOLEAN      _readOnly
        AS UINTEGER     _scrollWidth
        AS UINTEGER     _selected
        AS UINTEGER     _spacing

END TYPE

DECLARE SUB      GTGADGETS DEPLOY
DECLARE SUB      GTGADGETS FREE

DECLARE SUB      GTG DRAW BEVEL BOX (_COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), BYVAL recessed AS BOOLEAN = FALSE)

DECLARE FUNCTION GTGADGETS_NEXT_ID AS INTEGER

