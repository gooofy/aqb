REM gadtools.library constants (no types, no other OS module deps)

OPTION EXPLICIT

OPTION PRIVATE

PUBLIC CONST AS ULONG GENERIC_KIND  = 0
PUBLIC CONST AS ULONG BUTTON_KIND   = 1
PUBLIC CONST AS ULONG CHECKBOX_KIND = 2
PUBLIC CONST AS ULONG INTEGER_KIND  = 3
PUBLIC CONST AS ULONG LISTVIEW_KIND = 4
PUBLIC CONST AS ULONG MX_KIND       = 5
PUBLIC CONST AS ULONG NUMBER_KIND   = 6
PUBLIC CONST AS ULONG CYCLE_KIND    = 7
PUBLIC CONST AS ULONG PALETTE_KIND  = 8
PUBLIC CONST AS ULONG SCROLLER_KIND = 9
PUBLIC CONST AS ULONG SLIDER_KIND   = 11
PUBLIC CONST AS ULONG STRING_KIND   = 12
PUBLIC CONST AS ULONG TEXT_KIND     = 13

PUBLIC CONST AS ULONG PLACETEXT_LEFT  = &H0001
PUBLIC CONST AS ULONG PLACETEXT_RIGHT = &H0002
PUBLIC CONST AS ULONG PLACETEXT_ABOVE = &H0004
PUBLIC CONST AS ULONG PLACETEXT_BELOW = &H0008
PUBLIC CONST AS ULONG PLACETEXT_IN    = &H0010
PUBLIC CONST AS ULONG NG_HIGHLABEL    = &H0020

PUBLIC CONST AS UBYTE MENU_IMAGE = 128
PUBLIC CONST AS UBYTE NM_TITLE   = 1
PUBLIC CONST AS UBYTE NM_ITEM    = 2
PUBLIC CONST AS UBYTE NM_SUB     = 3
PUBLIC CONST AS UBYTE NM_END     = 0
PUBLIC CONST AS UBYTE NM_IGNORE  = 64

PUBLIC CONST AS VOID PTR NM_BARLABEL = -1

REM stolen from intuition, repeated here as private consts to avoid dependency
CONST AS UINTEGER MENUENABLED     = &H0001
CONST AS UINTEGER ITEMENABLED     = &H0010
CONST AS UINTEGER COMMSEQ         = &H0004

CONST AS ULONG IDCMP_GADGETUP     = &H00000040
CONST AS ULONG IDCMP_MOUSEBUTTONS = &H00000008
CONST AS ULONG IDCMP_MOUSEMOVE    = &H00000010
CONST AS ULONG IDCMP_GADGETDOWN   = &H00000020
CONST AS ULONG IDCMP_INTUITICKS   = &H00400000

PUBLIC CONST AS UINTEGER NM_MENUDISABLED  = MENUENABLED
PUBLIC CONST AS UINTEGER NM_ITEMDISABLED  = ITEMENABLED
PUBLIC CONST AS UINTEGER NM_COMMANDSTRING = COMMSEQ

PUBLIC CONST AS ULONG GTMENU_TRIMMED = &H00000001
PUBLIC CONST AS ULONG GTMENU_INVALID = &H00000002
PUBLIC CONST AS ULONG GTMENU_NOMEM   = &H00000003

PUBLIC CONST AS UINTEGER MX_WIDTH        = 17
PUBLIC CONST AS UINTEGER MX_HEIGHT       =  9
PUBLIC CONST AS UINTEGER CHECKBOX_WIDTH  = 26
PUBLIC CONST AS UINTEGER CHECKBOX_HEIGHT = 11

REM stolen from utility, repeated here as private consts to avoid dependency
CONST AS ULONG TAG_USER = &H80000000

PUBLIC CONST AS ULONG GT_TagBase               = TAG_USER + &H80000
PUBLIC CONST AS ULONG GTVI_NewWindow           = GT_TagBase+1
PUBLIC CONST AS ULONG GTVI_NWTags              = GT_TagBase+2
PUBLIC CONST AS ULONG GT_Private0              = GT_TagBase+3
PUBLIC CONST AS ULONG GTCB_Checked             = GT_TagBase+4
PUBLIC CONST AS ULONG GTLV_Top                 = GT_TagBase+5
PUBLIC CONST AS ULONG GTLV_Labels              = GT_TagBase+6
PUBLIC CONST AS ULONG GTLV_ReadOnly            = GT_TagBase+7
PUBLIC CONST AS ULONG GTLV_ScrollWidth         = GT_TagBase+8
PUBLIC CONST AS ULONG GTMX_Labels              = GT_TagBase+9
PUBLIC CONST AS ULONG GTMX_Active              = GT_TagBase+10
PUBLIC CONST AS ULONG GTTX_Text                = GT_TagBase+11
PUBLIC CONST AS ULONG GTTX_CopyText            = GT_TagBase+12
PUBLIC CONST AS ULONG GTNM_Number              = GT_TagBase+13
PUBLIC CONST AS ULONG GTCY_Labels              = GT_TagBase+14
PUBLIC CONST AS ULONG GTCY_Active              = GT_TagBase+15
PUBLIC CONST AS ULONG GTPA_Depth               = GT_TagBase+16
PUBLIC CONST AS ULONG GTPA_Color               = GT_TagBase+17
PUBLIC CONST AS ULONG GTPA_ColorOffset         = GT_TagBase+18
PUBLIC CONST AS ULONG GTPA_IndicatorWidth      = GT_TagBase+19
PUBLIC CONST AS ULONG GTPA_IndicatorHeight     = GT_TagBase+20
PUBLIC CONST AS ULONG GTSC_Top                 = GT_TagBase+21
PUBLIC CONST AS ULONG GTSC_Total               = GT_TagBase+22
PUBLIC CONST AS ULONG GTSC_Visible             = GT_TagBase+23
PUBLIC CONST AS ULONG GTSC_Overlap             = GT_TagBase+24
PUBLIC CONST AS ULONG GTSL_Min                 = GT_TagBase+38
PUBLIC CONST AS ULONG GTSL_Max                 = GT_TagBase+39
PUBLIC CONST AS ULONG GTSL_Level               = GT_TagBase+40
PUBLIC CONST AS ULONG GTSL_MaxLevelLen         = GT_TagBase+41
PUBLIC CONST AS ULONG GTSL_LevelFormat         = GT_TagBase+42
PUBLIC CONST AS ULONG GTSL_LevelPlace          = GT_TagBase+43
PUBLIC CONST AS ULONG GTSL_DispFunc            = GT_TagBase+44
PUBLIC CONST AS ULONG GTST_String              = GT_TagBase+45
PUBLIC CONST AS ULONG GTST_MaxChars            = GT_TagBase+46
PUBLIC CONST AS ULONG GTIN_Number              = GT_TagBase+47
PUBLIC CONST AS ULONG GTIN_MaxChars            = GT_TagBase+48
PUBLIC CONST AS ULONG GTMN_TextAttr            = GT_TagBase+49
PUBLIC CONST AS ULONG GTMN_FrontPen            = GT_TagBase+50
PUBLIC CONST AS ULONG GTBB_Recessed            = GT_TagBase+51
PUBLIC CONST AS ULONG GT_VisualInfo            = GT_TagBase+52
PUBLIC CONST AS ULONG GTLV_ShowSelected        = GT_TagBase+53
PUBLIC CONST AS ULONG GTLV_Selected            = GT_TagBase+54
PUBLIC CONST AS ULONG GT_Reserved1             = GT_TagBase+56
PUBLIC CONST AS ULONG GTTX_Border              = GT_TagBase+57
PUBLIC CONST AS ULONG GTNM_Border              = GT_TagBase+58
PUBLIC CONST AS ULONG GTSC_Arrows              = GT_TagBase+59
PUBLIC CONST AS ULONG GTMN_Menu                = GT_TagBase+60
PUBLIC CONST AS ULONG GTMX_Spacing             = GT_TagBase+61
PUBLIC CONST AS ULONG GTMN_FullMenu            = GT_TagBase+62
PUBLIC CONST AS ULONG GTMN_SecondaryError      = GT_TagBase+63
PUBLIC CONST AS ULONG GT_Underscore            = GT_TagBase+64
PUBLIC CONST AS ULONG GTST_EditHook            = GT_TagBase+55
PUBLIC CONST AS ULONG GTIN_EditHook            = GTST_EditHook
PUBLIC CONST AS ULONG GTMN_Checkmark           = GT_TagBase+65
PUBLIC CONST AS ULONG GTMN_AmigaKey            = GT_TagBase+66
PUBLIC CONST AS ULONG GTMN_NewLookMenus        = GT_TagBase+67
PUBLIC CONST AS ULONG GTCB_Scaled              = GT_TagBase+68
PUBLIC CONST AS ULONG GTMX_Scaled              = GT_TagBase+69
PUBLIC CONST AS ULONG GTPA_NumColors           = GT_TagBase+70
PUBLIC CONST AS ULONG GTMX_TitlePlace          = GT_TagBase+71
PUBLIC CONST AS ULONG GTTX_FrontPen            = GT_TagBase+72
PUBLIC CONST AS ULONG GTTX_BackPen             = GT_TagBase+73
PUBLIC CONST AS ULONG GTTX_Justification       = GT_TagBase+74
PUBLIC CONST AS ULONG GTNM_FrontPen            = GT_TagBase+72
PUBLIC CONST AS ULONG GTNM_BackPen             = GT_TagBase+73
PUBLIC CONST AS ULONG GTNM_Justification       = GT_TagBase+74
PUBLIC CONST AS ULONG GTNM_Format              = GT_TagBase+75
PUBLIC CONST AS ULONG GTNM_MaxNumberLen        = GT_TagBase+76
PUBLIC CONST AS ULONG GTBB_FrameType           = GT_TagBase+77
PUBLIC CONST AS ULONG GTLV_MakeVisible         = GT_TagBase+78
PUBLIC CONST AS ULONG GTLV_ItemHeight          = GT_TagBase+79
PUBLIC CONST AS ULONG GTSL_MaxPixelLen         = GT_TagBase+80
PUBLIC CONST AS ULONG GTSL_Justification       = GT_TagBase+81
PUBLIC CONST AS ULONG GTPA_ColorTable          = GT_TagBase+82
PUBLIC CONST AS ULONG GTLV_CallBack            = GT_TagBase+83
PUBLIC CONST AS ULONG GTLV_MaxPen              = GT_TagBase+84
PUBLIC CONST AS ULONG GTTX_Clipped             = GT_TagBase+85
PUBLIC CONST AS ULONG GTNM_Clipped             = GT_TagBase+85
PUBLIC CONST AS ULONG GT_Reserved0             = GTST_EditHook

PUBLIC CONST AS ULONG GTJ_LEFT         = 0
PUBLIC CONST AS ULONG GTJ_RIGHT        = 1
PUBLIC CONST AS ULONG GTJ_CENTER       = 2

PUBLIC CONST AS ULONG BBFT_BUTTON      = 1
PUBLIC CONST AS ULONG BBFT_RIDGE       = 2
PUBLIC CONST AS ULONG BBFT_ICONDROPBOX = 3

PUBLIC CONST AS UINTEGER INTERWIDTH    = 8
PUBLIC CONST AS UINTEGER INTERHEIGHT   = 4

PUBLIC CONST AS ULONG ARROWIDCMP       = IDCMP_GADGETUP OR IDCMP_GADGETDOWN OR IDCMP_INTUITICKS OR IDCMP_MOUSEBUTTONS

PUBLIC CONST AS ULONG BUTTONIDCMP      = IDCMP_GADGETUP
PUBLIC CONST AS ULONG CHECKBOXIDCMP    = IDCMP_GADGETUP
PUBLIC CONST AS ULONG INTEGERIDCMP     = IDCMP_GADGETUP
PUBLIC CONST AS ULONG LISTVIEWIDCMP    = IDCMP_GADGETUP OR IDCMP_GADGETDOWN OR IDCMP_MOUSEMOVE OR ARROWIDCMP

PUBLIC CONST AS ULONG MXIDCMP          = IDCMP_GADGETDOWN
PUBLIC CONST AS ULONG NUMBERIDCMP      = 0
PUBLIC CONST AS ULONG CYCLEIDCMP       = IDCMP_GADGETUP
PUBLIC CONST AS ULONG PALETTEIDCMP     = IDCMP_GADGETUP

PUBLIC CONST AS ULONG SCROLLERIDCMP    = IDCMP_GADGETUP OR IDCMP_GADGETDOWN OR IDCMP_MOUSEMOVE
PUBLIC CONST AS ULONG SLIDERIDCMP      = IDCMP_GADGETUP OR IDCMP_GADGETDOWN OR IDCMP_MOUSEMOVE
PUBLIC CONST AS ULONG STRINGIDCMP      = IDCMP_GADGETUP

PUBLIC CONST AS ULONG TEXTIDCMP        = 0

PUBLIC CONST AS ULONG NWAY_KIND        = CYCLE_KIND
PUBLIC CONST AS ULONG NWAYIDCMP        = CYCLEIDCMP
PUBLIC CONST AS ULONG GTNW_Labels      = GTCY_Labels
PUBLIC CONST AS ULONG GTNW_Active      = GTCY_Active

PUBLIC CONST AS ULONG LV_DRAW              = &H202
PUBLIC CONST AS ULONG LVCB_OK              = 0
PUBLIC CONST AS ULONG LVCB_UNKNOWN         = 1
PUBLIC CONST AS ULONG LVR_NORMAL           = 0
PUBLIC CONST AS ULONG LVR_SELECTED         = 1
PUBLIC CONST AS ULONG LVR_NORMALDISABLED   = 2
PUBLIC CONST AS ULONG LVR_SELECTEDDISABLED = 8
