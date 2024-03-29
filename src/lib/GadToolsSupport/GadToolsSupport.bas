OPTION EXPLICIT

IMPORT OSGadTools
IMPORT Collections

' --------------------------------------------------------------------------------------------------------
' --
' -- ERROR codes for GadToolsSupport 4xx
' --
' --------------------------------------------------------------------------------------------------------

CONST AE_GTG_CREATE   AS INTEGER = 400
CONST AE_GTG_MODIFY   AS INTEGER = 401
CONST AE_GTG_DEPLOY   AS INTEGER = 402
CONST AE_GTG_SELECTED AS INTEGER = 403
CONST AE_GTG_CALLBACK AS INTEGER = 404
CONST AE_GTG_BUFFER   AS INTEGER = 405
CONST AE_GTG_NUM      AS INTEGER = 406

' --------------------------------------------------------------------------------------------------------
' --
' -- GadTools Gadgets OOP Wrappers
' --
' --------------------------------------------------------------------------------------------------------

CLASS CGTGadget

    PUBLIC:

        DECLARE EXTERN CONSTRUCTOR (BYVAL label AS STRING, _
                                    _COORD2(BYVAL s1 AS BOOLEAN, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                                    BYVAL user_data AS ANY PTR =NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE EXTERN PROPERTY x1 AS INTEGER
        DECLARE EXTERN PROPERTY x1 (BYVAL x AS INTEGER)

        DECLARE EXTERN PROPERTY y1 AS INTEGER
        DECLARE EXTERN PROPERTY y1 (BYVAL y AS INTEGER)

        DECLARE EXTERN PROPERTY x2 AS INTEGER
        DECLARE EXTERN PROPERTY x2 (BYVAL x AS INTEGER)

        DECLARE EXTERN PROPERTY y2 AS INTEGER
        DECLARE EXTERN PROPERTY y2 (BYVAL y AS INTEGER)

        DECLARE EXTERN PROPERTY text AS STRING
        DECLARE EXTERN PROPERTY text (BYVAL txt AS STRING)

        DECLARE EXTERN PROPERTY id AS INTEGER
        DECLARE EXTERN PROPERTY id (BYVAL id AS INTEGER)

        DECLARE EXTERN PROPERTY flags AS ULONG
        DECLARE EXTERN PROPERTY flags (BYVAL f AS ULONG)

        DECLARE EXTERN PROPERTY deployed AS BOOLEAN

        AS SUB (BYVAL CGTGadget PTR, BYVAL UINTEGER) gadgetup_cb
        AS SUB (BYVAL CGTGadget PTR, BYVAL UINTEGER) gadgetdown_cb
        AS SUB (BYVAL CGTGadget PTR, BYVAL UINTEGER) gadgetmove_cb

        AS ANY PTR  user_data
        AS ULONG    underscore

    PRIVATE:

        AS CGTGadget PTR  prev, next

        AS NewGadget      ng

        AS Gadget PTR     gad
        AS Window PTR     win

        AS ANY PTR        deploy_cb


END CLASS

CLASS CGTButton EXTENDS CGTGadget

    PUBLIC:
        DECLARE EXTERN CONSTRUCTOR (BYVAL label AS STRING, _
                                    _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                                    BYVAL user_data AS ANY PTR =NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE EXTERN PROPERTY disabled AS BOOLEAN
        DECLARE EXTERN PROPERTY disabled (BYVAL b AS BOOLEAN)

        DECLARE EXTERN PROPERTY immediate AS BOOLEAN
        DECLARE EXTERN PROPERTY immediate (BYVAL b AS BOOLEAN)

    PRIVATE:
        AS BOOLEAN  _disabled
        AS BOOLEAN  _immediate

END CLASS

CLASS CGTCheckBox EXTENDS CGTGadget

    PUBLIC:
        DECLARE EXTERN CONSTRUCTOR (BYVAL label AS STRING, _
                                    _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                                    BYVAL user_data AS ANY PTR =NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE EXTERN PROPERTY disabled AS BOOLEAN
        DECLARE EXTERN PROPERTY disabled (BYVAL b AS BOOLEAN)

        DECLARE EXTERN PROPERTY checked AS BOOLEAN
        DECLARE EXTERN PROPERTY checked (BYVAL b AS BOOLEAN)

        DECLARE EXTERN PROPERTY scaled AS BOOLEAN
        DECLARE EXTERN PROPERTY scaled (BYVAL b AS BOOLEAN)

    PRIVATE:
        AS BOOLEAN  _disabled
        AS BOOLEAN  _checked
        AS BOOLEAN  _scaled

END CLASS

CLASS CGTSlider EXTENDS CGTGadget

    PUBLIC:

        DECLARE EXTERN CONSTRUCTOR (BYVAL label AS STRING, BYVAL min AS INTEGER, BYVAL max AS INTEGER, BYVAL level AS INTEGER, BYVAL freedom AS ULONG, _
                                    _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                                    BYVAL user_data AS ANY PTR =NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE EXTERN PROPERTY disabled AS BOOLEAN
        DECLARE EXTERN PROPERTY disabled (BYVAL b AS BOOLEAN)

        DECLARE EXTERN PROPERTY min AS INTEGER
        DECLARE EXTERN PROPERTY min (BYVAL i AS INTEGER)

        DECLARE EXTERN PROPERTY max AS INTEGER
        DECLARE EXTERN PROPERTY max (BYVAL i AS INTEGER)

        DECLARE EXTERN PROPERTY level AS INTEGER
        DECLARE EXTERN PROPERTY level (BYVAL i AS INTEGER)

        DECLARE EXTERN PROPERTY maxLevelLen AS INTEGER
        DECLARE EXTERN PROPERTY maxLevelLen (BYVAL i AS INTEGER)

        DECLARE EXTERN PROPERTY levelFormat AS STRING
        DECLARE EXTERN PROPERTY levelFormat (BYVAL s AS STRING)

        DECLARE EXTERN PROPERTY levelPlace AS ULONG
        DECLARE EXTERN PROPERTY levelPlace (BYVAL u AS ULONG)

        DECLARE EXTERN PROPERTY immediate AS BOOLEAN
        DECLARE EXTERN PROPERTY immediate (BYVAL b AS BOOLEAN)

        DECLARE EXTERN PROPERTY relVerify AS BOOLEAN
        DECLARE EXTERN PROPERTY relVerify (BYVAL b AS BOOLEAN)

        DECLARE EXTERN PROPERTY freedom AS ULONG
        DECLARE EXTERN PROPERTY freedom (BYVAL u AS ULONG)

    PRIVATE:
        AS BOOLEAN  _disabled
        AS INTEGER  _min, _max, _level
        AS ULONG    _freedom
        AS INTEGER  _maxLevelLen
        AS STRING   _levelFormat
        AS ULONG    _levelPlace
        AS BOOLEAN  _immediate, _relVerify

END CLASS

CLASS CGTText EXTENDS CGTGadget

    PUBLIC:
        DECLARE EXTERN CONSTRUCTOR ( BYVAL label AS STRING, BYVAL text AS STRING, _
                                     _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                                     BYVAL user_data AS ANY PTR =NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE EXTERN PROPERTY text AS STRING
        DECLARE EXTERN PROPERTY text (BYVAL value AS STRING)

        DECLARE EXTERN PROPERTY copyText AS BOOLEAN
        DECLARE EXTERN PROPERTY copyText (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY border AS BOOLEAN
        DECLARE EXTERN PROPERTY border (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY frontPen AS UBYTE
        DECLARE EXTERN PROPERTY frontPen (BYVAL value AS UBYTE)

        DECLARE EXTERN PROPERTY backPen AS UBYTE
        DECLARE EXTERN PROPERTY backPen (BYVAL value AS UBYTE)

        DECLARE EXTERN PROPERTY justification AS UBYTE
        DECLARE EXTERN PROPERTY justification (BYVAL value AS UBYTE)

        DECLARE EXTERN PROPERTY clipped AS BOOLEAN
        DECLARE EXTERN PROPERTY clipped (BYVAL value AS BOOLEAN)

    PRIVATE:

        AS STRING       _text
        AS BOOLEAN      _copyText
        AS BOOLEAN      _border
        AS UBYTE        _frontPen
        AS UBYTE        _backPen
        AS UBYTE        _justification
        AS BOOLEAN      _clipped

END CLASS

CLASS CGTScroller EXTENDS CGTGadget

    PUBLIC:
        DECLARE EXTERN CONSTRUCTOR ( BYVAL label AS STRING, BYVAL top AS INTEGER, BYVAL total AS INTEGER, BYVAL visible AS INTEGER, BYVAL freedom AS ULONG,  _
                                     _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                                     BYVAL user_data AS ANY PTR =NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE EXTERN PROPERTY disabled AS BOOLEAN
        DECLARE EXTERN PROPERTY disabled (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY relVerify AS BOOLEAN
        DECLARE EXTERN PROPERTY relVerify (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY immediate AS BOOLEAN
        DECLARE EXTERN PROPERTY immediate (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY top AS INTEGER
        DECLARE EXTERN PROPERTY top (BYVAL value AS INTEGER)

        DECLARE EXTERN PROPERTY total AS INTEGER
        DECLARE EXTERN PROPERTY total (BYVAL value AS INTEGER)

        DECLARE EXTERN PROPERTY visible AS BOOLEAN
        DECLARE EXTERN PROPERTY visible (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY arrows AS UINTEGER
        DECLARE EXTERN PROPERTY arrows (BYVAL value AS UINTEGER)

        DECLARE EXTERN PROPERTY freedom AS ULONG
        DECLARE EXTERN PROPERTY freedom (BYVAL value AS ULONG)

    PRIVATE:

        AS BOOLEAN      _disabled
        AS BOOLEAN      _relVerify
        AS BOOLEAN      _immediate
        AS INTEGER      _top
        AS INTEGER      _total
        AS BOOLEAN      _visible
        AS UINTEGER     _arrows
        AS ULONG        _freedom

END CLASS

CLASS CGTString EXTENDS CGTGadget

    PUBLIC:
        DECLARE EXTERN CONSTRUCTOR ( BYVAL label AS STRING,  _
                                     _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                                     BYVAL user_data AS ANY PTR =NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE EXTERN PROPERTY disabled AS BOOLEAN
        DECLARE EXTERN PROPERTY disabled (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY immediate AS BOOLEAN
        DECLARE EXTERN PROPERTY immediate (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY tabCycle AS BOOLEAN
        DECLARE EXTERN PROPERTY tabCycle (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY str AS STRING
        DECLARE EXTERN PROPERTY str (BYVAL value AS STRING)

        DECLARE EXTERN PROPERTY maxChars AS UINTEGER
        DECLARE EXTERN PROPERTY maxChars (BYVAL value AS UINTEGER)

        DECLARE EXTERN PROPERTY exitHelp AS BOOLEAN
        DECLARE EXTERN PROPERTY exitHelp (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY justification AS STRING
        DECLARE EXTERN PROPERTY justification (BYVAL value AS STRING)

        DECLARE EXTERN PROPERTY replaceMode AS BOOLEAN
        DECLARE EXTERN PROPERTY replaceMode (BYVAL value AS BOOLEAN)

    PRIVATE:

        AS BOOLEAN      _disabled
        AS BOOLEAN      _immediate
        AS BOOLEAN      _tabCycle
        AS STRING       _str
        AS UINTEGER     _maxChars
        AS BOOLEAN      _exitHelp
        AS STRING       _justification
        AS BOOLEAN      _replaceMode

END CLASS

CLASS CGTInteger EXTENDS CGTGadget

    PUBLIC:
        DECLARE EXTERN CONSTRUCTOR ( BYVAL label AS STRING,  _
                                     _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                                     BYVAL user_data AS ANY PTR =NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE EXTERN PROPERTY disabled AS BOOLEAN
        DECLARE EXTERN PROPERTY disabled (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY immediate AS BOOLEAN
        DECLARE EXTERN PROPERTY immediate (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY tabCycle AS BOOLEAN
        DECLARE EXTERN PROPERTY tabCycle (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY number AS LONG
        DECLARE EXTERN PROPERTY number (BYVAL value AS LONG)

        DECLARE EXTERN PROPERTY maxChars AS UINTEGER
        DECLARE EXTERN PROPERTY maxChars (BYVAL value AS UINTEGER)

        DECLARE EXTERN PROPERTY exitHelp AS BOOLEAN
        DECLARE EXTERN PROPERTY exitHelp (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY justification AS STRING
        DECLARE EXTERN PROPERTY justification (BYVAL value AS STRING)

        DECLARE EXTERN PROPERTY replaceMode AS BOOLEAN
        DECLARE EXTERN PROPERTY replaceMode (BYVAL value AS BOOLEAN)

    PRIVATE:

        AS BOOLEAN      _disabled
        AS BOOLEAN      _immediate
        AS BOOLEAN      _tabCycle
        AS LONG         _number
        AS UINTEGER     _maxChars
        AS BOOLEAN      _exitHelp
        AS STRING       _justification
        AS BOOLEAN      _replaceMode

END CLASS

CLASS CGTNumber EXTENDS CGTGadget

    PUBLIC:
        DECLARE EXTERN CONSTRUCTOR ( BYVAL label AS STRING, BYVAL number AS LONG,  _
                                     _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                                     BYVAL user_data AS ANY PTR =NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE EXTERN PROPERTY number AS LONG
        DECLARE EXTERN PROPERTY number (BYVAL value AS LONG)

        DECLARE EXTERN PROPERTY border AS BOOLEAN
        DECLARE EXTERN PROPERTY border (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY frontPen AS UBYTE
        DECLARE EXTERN PROPERTY frontPen (BYVAL value AS UBYTE)

        DECLARE EXTERN PROPERTY backPen AS UBYTE
        DECLARE EXTERN PROPERTY backPen (BYVAL value AS UBYTE)

        DECLARE EXTERN PROPERTY justification AS UBYTE
        DECLARE EXTERN PROPERTY justification (BYVAL value AS UBYTE)

        DECLARE EXTERN PROPERTY format AS STRING
        DECLARE EXTERN PROPERTY format (BYVAL value AS STRING)

        DECLARE EXTERN PROPERTY maxNumberLen AS ULONG
        DECLARE EXTERN PROPERTY maxNumberLen (BYVAL value AS ULONG)

        DECLARE EXTERN PROPERTY clipped AS BOOLEAN
        DECLARE EXTERN PROPERTY clipped (BYVAL value AS BOOLEAN)

    PRIVATE:

        AS LONG         _number
        AS BOOLEAN      _border
        AS UBYTE        _frontPen
        AS UBYTE        _backPen
        AS UBYTE        _justification
        AS STRING       _format
        AS ULONG        _maxNumberLen
        AS BOOLEAN      _clipped

END CLASS

CLASS CGTMX EXTENDS CGTGadget

    PUBLIC:
        DECLARE EXTERN CONSTRUCTOR ( BYVAL label AS STRING, BYVAL labels AS STRING PTR,  _
                                     _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                                     BYVAL user_data AS ANY PTR =NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE EXTERN PROPERTY disabled AS BOOLEAN
        DECLARE EXTERN PROPERTY disabled (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY labels AS STRING PTR
        DECLARE EXTERN PROPERTY labels (BYVAL value AS STRING PTR)

        DECLARE EXTERN PROPERTY active AS UINTEGER
        DECLARE EXTERN PROPERTY active (BYVAL value AS UINTEGER)

        DECLARE EXTERN PROPERTY spacing AS UINTEGER
        DECLARE EXTERN PROPERTY spacing (BYVAL value AS UINTEGER)

        DECLARE EXTERN PROPERTY scaled AS BOOLEAN
        DECLARE EXTERN PROPERTY scaled (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY titlePlace AS ULONG
        DECLARE EXTERN PROPERTY titlePlace (BYVAL value AS ULONG)

    PRIVATE:

        AS BOOLEAN      _disabled
        AS STRING PTR   _labels
        AS UINTEGER     _active
        AS UINTEGER     _spacing
        AS BOOLEAN      _scaled
        AS ULONG        _titlePlace

END CLASS

CLASS CGTCycle EXTENDS CGTGadget

    PUBLIC:
        DECLARE EXTERN CONSTRUCTOR ( BYVAL label AS STRING, BYVAL labels AS STRING PTR,  _
                                     _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                                     BYVAL user_data AS ANY PTR =NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE EXTERN PROPERTY disabled AS BOOLEAN
        DECLARE EXTERN PROPERTY disabled (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY labels AS STRING PTR
        DECLARE EXTERN PROPERTY labels (BYVAL value AS STRING PTR)

        DECLARE EXTERN PROPERTY active AS UINTEGER
        DECLARE EXTERN PROPERTY active (BYVAL value AS UINTEGER)

    PRIVATE:

        AS BOOLEAN      _disabled
        AS STRING PTR   _labels
        AS UINTEGER     _active

END CLASS

CLASS CGTPalette EXTENDS CGTGadget

    PUBLIC:
        DECLARE EXTERN CONSTRUCTOR ( BYVAL label AS STRING, BYVAL numColors AS UINTEGER,  _
                                     _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                                     BYVAL user_data AS ANY PTR =NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE EXTERN PROPERTY disabled AS BOOLEAN
        DECLARE EXTERN PROPERTY disabled (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY color AS UBYTE
        DECLARE EXTERN PROPERTY color (BYVAL value AS UBYTE)

        DECLARE EXTERN PROPERTY colorOffset AS UBYTE
        DECLARE EXTERN PROPERTY colorOffset (BYVAL value AS UBYTE)

        DECLARE EXTERN PROPERTY indicatorWidth AS UINTEGER
        DECLARE EXTERN PROPERTY indicatorWidth (BYVAL value AS UINTEGER)

        DECLARE EXTERN PROPERTY indicatorHeight AS UINTEGER
        DECLARE EXTERN PROPERTY indicatorHeight (BYVAL value AS UINTEGER)

        DECLARE EXTERN PROPERTY colorTable AS UBYTE PTR
        DECLARE EXTERN PROPERTY colorTable (BYVAL value AS UBYTE PTR)

        DECLARE EXTERN PROPERTY numColors AS UINTEGER
        DECLARE EXTERN PROPERTY numColors (BYVAL value AS UINTEGER)

    PRIVATE:

        AS BOOLEAN      _disabled
        AS UBYTE        _color
        AS UBYTE        _colorOffset
        AS UINTEGER     _indicatorWidth
        AS UINTEGER     _indicatorHeight
        AS UBYTE PTR    _colorTable
        AS UINTEGER     _numColors

END CLASS

CLASS CGTListView EXTENDS CGTGadget

    PUBLIC:
        DECLARE EXTERN CONSTRUCTOR ( BYVAL label AS STRING, BYVAL labels AS CExecList PTR,  _
                                     _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _
                                     BYVAL user_data AS ANY PTR =NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)

        DECLARE EXTERN PROPERTY disabled AS BOOLEAN
        DECLARE EXTERN PROPERTY disabled (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY makeVisible AS INTEGER
        DECLARE EXTERN PROPERTY makeVisible (BYVAL value AS INTEGER)

        DECLARE EXTERN PROPERTY labels AS CExecList PTR
        DECLARE EXTERN PROPERTY labels (BYVAL value AS CExecList PTR)

        DECLARE EXTERN PROPERTY readOnly AS BOOLEAN
        DECLARE EXTERN PROPERTY readOnly (BYVAL value AS BOOLEAN)

        DECLARE EXTERN PROPERTY scrollWidth AS UINTEGER
        DECLARE EXTERN PROPERTY scrollWidth (BYVAL value AS UINTEGER)

        DECLARE EXTERN PROPERTY selected AS UINTEGER
        DECLARE EXTERN PROPERTY selected (BYVAL value AS UINTEGER)

        DECLARE EXTERN PROPERTY spacing AS UINTEGER
        DECLARE EXTERN PROPERTY spacing (BYVAL value AS UINTEGER)

    PRIVATE:

        AS BOOLEAN       _disabled
        AS INTEGER       _makeVisible
        AS CExecList PTR _labels
        AS BOOLEAN       _readOnly
        AS UINTEGER      _scrollWidth
        AS UINTEGER      _selected
        AS UINTEGER      _spacing

END CLASS

DECLARE EXTERN SUB      GTGADGETS DEPLOY
DECLARE EXTERN SUB      GTGADGETS FREE

' --------------------------------------------------------------------------------------------------------
' --
' -- Misc
' --
' --------------------------------------------------------------------------------------------------------

DECLARE EXTERN SUB      GTG DRAW BEVEL BOX (_COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), BYVAL recessed AS BOOLEAN = FALSE)

DECLARE EXTERN FUNCTION GTGADGETS_NEXT_ID AS INTEGER

