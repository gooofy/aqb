OPTION EXPLICIT

IMPORT OSIntuition

' --------------------------------------------------------------------------------------------------------
' --
' -- ERROR codes for IntuiSupport 2xx
' --
' --------------------------------------------------------------------------------------------------------

CONST AE_MENU_DEPLOYED        AS INTEGER = 200
CONST AE_MENU_ITEXT_VS_IMAGE  AS INTEGER = 201
CONST AE_MENU_NO_WINDOW       AS INTEGER = 202

' --------------------------------------------------------------------------------------------------------
' --
' -- Intuition Graphic Objects OOP Support
' --
' --------------------------------------------------------------------------------------------------------

CLASS CIntuiText

    PUBLIC:

        DECLARE EXTERN CONSTRUCTOR (BYVAL text AS string)

        DECLARE EXTERN PROPERTY frontPen AS UBYTE
        DECLARE EXTERN PROPERTY frontPen (BYVAL b AS UBYTE)

        DECLARE EXTERN PROPERTY backPen AS UBYTE
        DECLARE EXTERN PROPERTY backPen (BYVAL b AS UBYTE)

        DECLARE EXTERN PROPERTY drawMode AS UBYTE
        DECLARE EXTERN PROPERTY drawMode (BYVAL b AS UBYTE)

        DECLARE EXTERN PROPERTY leftEdge AS INTEGER
        DECLARE EXTERN PROPERTY leftEdge (BYVAL b AS INTEGER)

        DECLARE EXTERN PROPERTY topEdge AS INTEGER
        DECLARE EXTERN PROPERTY topEdge (BYVAL b AS INTEGER)

        DECLARE EXTERN PROPERTY textFont AS TextAttr PTR
        DECLARE EXTERN PROPERTY textFont (BYVAL b AS TextAttr PTR)

        DECLARE EXTERN PROPERTY text AS string
        DECLARE EXTERN PROPERTY text (BYVAL b AS string)

        DECLARE EXTERN PROPERTY nextText AS CIntuiText PTR
        DECLARE EXTERN PROPERTY nextText (BYVAL b AS CIntuiText PTR)

        DECLARE EXTERN PROPERTY intuiText AS IntuiText PTR

    PRIVATE:

        AS IntuiText PTR _intuiText

END CLASS

' --------------------------------------------------------------------------------------------------------
' --
' -- Intuition Menus OOP Support
' --
' --------------------------------------------------------------------------------------------------------

TYPE MenuItemUD
    AS MenuItem      _item
    AS CMenuItem PTR _wrapper   ' link back to the CMenuItem object that owns this MenuItem
END TYPE

' base class for all menu items
CLASS CMenuItem

    PUBLIC:

        DECLARE EXTERN CONSTRUCTOR (BYVAL parent AS CMenu PTR=NULL, BYVAL userData AS ANY = 0)

        DECLARE EXTERN PROPERTY CheckIt AS BOOLEAN
        DECLARE EXTERN PROPERTY CheckIt (BYVAL b AS BOOLEAN)

        DECLARE EXTERN PROPERTY Checked AS BOOLEAN
        DECLARE EXTERN PROPERTY Checked (BYVAL b AS BOOLEAN)

        DECLARE EXTERN PROPERTY Toggle AS BOOLEAN
        DECLARE EXTERN PROPERTY Toggle (BYVAL b AS BOOLEAN)

        DECLARE EXTERN PROPERTY Enabled AS BOOLEAN
        DECLARE EXTERN PROPERTY Enabled (BYVAL b AS BOOLEAN)

        DECLARE EXTERN PROPERTY Command AS BYTE
        DECLARE EXTERN PROPERTY Command (BYVAL b AS BYTE)

        DECLARE EXTERN PROPERTY MutualExclude AS LONG
        DECLARE EXTERN PROPERTY MutualExclude (BYVAL b AS LONG)

        DECLARE EXTERN PROPERTY HighFlags AS UINTEGER
        DECLARE EXTERN PROPERTY HighFlags (BYVAL b AS UINTEGER)

        DECLARE EXTERN SUB AddSubItem (BYVAL subItem AS CMenuItem PTR)
        DECLARE EXTERN SUB RemoveSubItems ()

        DECLARE EXTERN PROPERTY NextItem AS CMenuItem PTR
        DECLARE EXTERN PROPERTY NextItem (BYVAL i AS CMenuItem PTR)

        DECLARE EXTERN PROPERTY Parent AS CMenu PTR

        DECLARE EXTERN SUB BBox (BYREF x1 AS INTEGER, BYREF y1 AS INTEGER, BYREF x2 AS INTEGER, BYREF y2 AS INTEGER)

        AS SUB (BYVAL CMenuItem PTR) cb

    PUBLIC:

        AS ANY           _userData

    PROTECTED:

        AS CMenu PTR     _parent
        AS MenuItemUD    _item
        AS CMenuItem PTR _subItem
        AS CMenuItem PTR _nextItem

END CLASS

CLASS CMenuItemText EXTENDS CMenuItem

    PUBLIC:

        DECLARE EXTERN CONSTRUCTOR (BYVAL text AS string, BYVAL parent AS CMenu PTR=NULL, BYVAL command AS BYTE=0, BYVAL userData AS ANY = 0)

        DECLARE EXTERN PROPERTY text AS string
        DECLARE EXTERN PROPERTY text (BYVAL s AS string)

        DECLARE EXTERN PROPERTY textSelected AS string
        DECLARE EXTERN PROPERTY textSelected (BYVAL s AS string)

        DECLARE EXTERN PROPERTY iText AS IntuiText PTR
        DECLARE EXTERN PROPERTY iText (BYVAL s AS IntuiText PTR)

        DECLARE EXTERN PROPERTY iTextSelected AS IntuiText PTR
        DECLARE EXTERN PROPERTY iTextSelected (BYVAL s AS IntuiText PTR)

END CLASS

CLASS CMenuItemSeparator EXTENDS CMenuItem

    PUBLIC:

        DECLARE EXTERN CONSTRUCTOR (BYVAL parent AS CMenu PTR, BYVAL userData AS ANY = 0)

END CLASS

CLASS CMenu

    PUBLIC:

        DECLARE EXTERN CONSTRUCTOR (BYVAL name AS STRING, BYVAL prevMenu AS CMenu PTR = NULL)

        DECLARE EXTERN PROPERTY nextMenu AS CMenu PTR
        DECLARE EXTERN PROPERTY nextMenu (BYVAL x AS CMenu PTR)

        DECLARE EXTERN PROPERTY firstItem AS CMenuItem PTR
        DECLARE EXTERN PROPERTY firstItem (BYVAL i AS CMenuItem PTR)

        DECLARE EXTERN PROPERTY enabled AS BOOLEAN
        DECLARE EXTERN PROPERTY enabled (BYVAL b AS BOOLEAN)

        DECLARE EXTERN SUB      addItem (BYVAL item AS CMenuItem PTR)
        DECLARE EXTERN SUB      removeAllItems ()

        DECLARE EXTERN SUB      deploy
        DECLARE EXTERN SUB      undeploy

    PRIVATE:

        AS Menu           _menu
        AS CMenuItem PTR  _firstItem
        AS CMenuItem PTR  _lastItem
        AS CMenu PTR      _nextMenu
        AS INTEGER        _win_id
        ' AS Window PTR     _window

END CLASS


