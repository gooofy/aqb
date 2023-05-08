REM Intuition Menu Tutorial 1

OPTION EXPLICIT

IMPORT IntuiSupport

SUB winCloseCB (BYVAL wid AS INTEGER, BYVAL ud AS ANY PTR)
    TRACE "window close cb called, wid=";wid
    SYSTEM
END SUB

SUB menuQuitCB (BYVAL item AS CMenuItem PTR)
    TRACE "menuQuitCB called"
    SYSTEM
END SUB

WINDOW 1, "Menu Tutorial 1"
ON WINDOW CLOSE CALL 1, winCloseCB

DIM AS CMenuItem PTR separator

DIM AS CMenu menu1 = CMenu ("Project")
DIM AS CMenuItemText      itemNew     = CMenuItemText ("New"    , @menu1, ASC("N"))
DIM AS CMenuItemText      itemOpen    = CMenuItemText ("Open...", @menu1, ASC("O"))
separator= NEW CMenuItemSeparator (@menu1)

DIM AS CMenuItemText      itemPrint   = CMenuItemText ("Print", @menu1)
itemPrint.addSubItem (NEW CMenuItemText ("NLQ"))
itemPrint.addSubItem (NEW CMenuItemText ("Draft"))

DIM AS CMenuItemText      itemFax     = CMenuItemText ("FAX", @menu1)
itemFax.Enabled = FALSE

separator = NEW CMenuItemSeparator (@menu1)
DIM AS CMenuItemText      itemQuit    = CMenuItemText ("Quit", @menu1, ASC("Q"))
itemQuit.cb = menuQuitCB

DIM AS CMenu menu2         = CMenu         ("Settings", @menu1)
DIM AS CMenuItemText item5 = CMenuItemText ("An item for the second menu", @menu2)

menu1.deploy()

PRINT "Right-click to access the menu"

WHILE TRUE
    SLEEP
WEND




