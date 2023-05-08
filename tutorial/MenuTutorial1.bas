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

DIM AS CMenu menu2 = CMenu ("Settings", @menu1)
DIM AS CMenuItemText itemCreateIcons = CMenuItemText ("Create Icons?", @menu2)
itemCreateIcons.CheckIt = TRUE : itemCreateIcons.Checked = TRUE : itemCreateIcons.Toggle = TRUE
separator= NEW CMenuItemSeparator (@menu2)
DIM AS CMenuItemText itemMX1 = CMenuItemText ("mx 1", @menu2)
itemMX1.CheckIt = TRUE : itemMX1.Checked = TRUE : itemMX1.Toggle = TRUE : itemMX1.MutualExclude = 8
DIM AS CMenuItemText itemMX2 = CMenuItemText ("mx 2", @menu2)
itemMX2.CheckIt = TRUE : itemMX2.Checked = FALSE : itemMX2.Toggle = TRUE : itemMX2.MutualExclude = 4

menu1.deploy()

PRINT "Right-click to access the menu"

WHILE TRUE
    SLEEP
WEND




