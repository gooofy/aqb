/*
**	$VER: gtlayout.h 47.1 (11.10.1999)
**	GadTools layout toolkit
**
**	Copyright © 1993-1999 by Olaf `Olsen' Barthel
**		Freely distributable.
*/

#ifndef _GTLAYOUT_H
#define _GTLAYOUT_H


/*****************************************************************************/


#ifndef INTUITION_GADGETCLASS_H
#include <intuition/gadgetclass.h>
#endif	/* !INTUITION_GADGETCLASS_H */

#ifndef LIBRARIES_GADTOOLS_H
#include <libraries/gadtools.h>
#endif	/* !LIBRARIES_GADTOOLS_H */


/*****************************************************************************/


/* Kinds of objects supported in addition to the normal GadTools kinds */
#define HORIZONTAL_KIND	45
#define VERTICAL_KIND	46
#define END_KIND	47
#define FRAME_KIND	48
#define BOX_KIND	49
#define FRACTION_KIND	50
#define XBAR_KIND	51
#define YBAR_KIND	52
#define PASSWORD_KIND	53
#define GAUGE_KIND	54
#define TAPEDECK_KIND	55
#define LEVEL_KIND	56
#define BOOPSI_KIND	57
#define POPUP_KIND	58
#define TAB_KIND	59
#define BLANK_KIND	60
#define IMAGE_KIND	61
#define TEXTEDIT_KIND	62


/*****************************************************************************/


/* Where to place a gadget label */
enum
{
	PLACE_Left,
	PLACE_Right,
	PLACE_Above,
	PLACE_In,
	PLACE_Below
};

/* How to align text lines in BOX_KIND gadgets */
enum
{
	ALIGNTEXT_Left,
	ALIGNTEXT_Centered,
	ALIGNTEXT_Right,
	ALIGNTEXT_Pad
};

/* The button images available for TAPEDECK_KIND gadgets. */
enum
{
	TDBT_Backward,
	TDBT_Forward,
	TDBT_Previous,
	TDBT_Next,
	TDBT_Stop,
	TDBT_Pause,
	TDBT_Record,
	TDBT_Rewind,
	TDBT_Eject,
	TDBT_Play,

	TDBTLAST
};

/* The frame types for groups. */
enum
{
	FRAMETYPE_None,
	FRAMETYPE_Label,
	FRAMETYPE_Tab
};

/* How to align the window opened by LT_Build() on the screen. */
#define ALIGNF_Right		(1 << 0)
#define ALIGNF_Left		(1 << 1)
#define ALIGNF_Top		(1 << 2)
#define ALIGNF_Bottom		(1 << 3)
#define ALIGNF_ExtraRight	(1 << 4)
#define ALIGNF_ExtraLeft	(1 << 5)
#define ALIGNF_ExtraTop		(1 << 6)
#define ALIGNF_ExtraBottom	(1 << 7)


/*****************************************************************************/


/* Generic tags, applicable for several object types */
#define LA_Chars		TAG_USER+2
#define LA_LabelPlace		TAG_USER+3
#define LA_ExtraSpace		TAG_USER+4
#define LA_NoKey		TAG_USER+30
#define LA_HighLabel		TAG_USER+31
#define LA_LabelText		TAG_USER+37
#define LA_LabelID		TAG_USER+38
#define LA_ID			TAG_USER+39
#define LA_Type			TAG_USER+40
#define LA_PageSelector		TAG_USER+79
#define LA_LabelChars		TAG_USER+107
#define LA_DefaultSize		TAG_USER+170
#define LA_LayoutSpace		TAG_USER+189

/* Storage type tags */
#define LA_BYTE			TAG_USER+63
#define LA_UBYTE		TAG_USER+64
#define LA_WORD			TAG_USER+65
#define LA_BOOL			TAG_USER+65
#define LA_UWORD		TAG_USER+66
#define LA_LONG			TAG_USER+67
#define LA_ULONG		TAG_USER+68
#define LA_STRPTR		TAG_USER+69

/* for use with LT_GetAttributes() only */
#define LA_Left			TAG_USER+16
#define LA_Top			TAG_USER+17
#define LA_Width		TAG_USER+18
#define LA_Height		TAG_USER+19
#define LA_LabelLeft		TAG_USER+114
#define LA_LabelTop		TAG_USER+115

/* BOOPSI_KIND */
#define LABO_TagCurrent		TAG_USER+119
#define LABO_TagTextAttr	TAG_USER+120
#define LABO_TagDrawInfo	TAG_USER+121
#define LABO_TagLink		TAG_USER+129
#define LABO_TagScreen		TAG_USER+132
#define LABO_Link		LALV_Link
#define LABO_ClassInstance	TAG_USER+122
#define LABO_ClassName		TAG_USER+123
#define LABO_ClassLibraryName	TAG_USER+124
#define LABO_ExactWidth		TAG_USER+127
#define LABO_ExactHeight	TAG_USER+128
#define LABO_RelFontHeight	TAG_USER+131
#define LABO_Object		TAG_USER+133
#define LABO_FullWidth		TAG_USER+135
#define LABO_FullHeight		TAG_USER+136
#define LABO_ActivateHook	TAG_USER+141

/* BOX_KIND */
#define LABX_Labels		TAG_USER+12
#define LABX_Lines		TAG_USER+13
#define LABX_Chars		TAG_USER+2
#define LABX_Rows		TAG_USER+1
#define LABX_Index		TAG_USER+14
#define LABX_Text		TAG_USER+15
#define LABX_AlignText		TAG_USER+27
#define LABX_DrawBox		TAG_USER+11
#define LABX_FirstLabel		TAG_USER+44
#define LABX_LastLabel		TAG_USER+45
#define LABX_ReserveSpace	TAG_USER+72
#define LABX_LabelTable		TAG_USER+98
#define LABX_FirstLine		TAG_USER+152
#define LABX_LastLine		TAG_USER+153
#define LABX_LineTable		TAG_USER+156
#define LABX_Line		TAG_USER+161
#define LABX_LineID		TAG_USER+162
#define LABX_TextPen		TAG_USER+172
#define LABX_BackPen		TAG_USER+173
#define LABX_Spacing		TAG_USER+180

/* BUTTON_KIND */
#define LABT_ReturnKey		TAG_USER+34
#define LABT_DefaultButton	TAG_USER+34
#define LABT_EscKey		TAG_USER+56
#define LABT_ExtraFat		TAG_USER+29
#define LABT_Lines		TAG_USER+140
#define LABT_FirstLine		TAG_USER+44
#define LABT_LastLine		TAG_USER+45
#define LABT_DefaultCorrection	TAG_USER+145
#define LABT_Smaller		TAG_USER+147

/* CYCLE_KIND */
#define LACY_FirstLabel		TAG_USER+44
#define LACY_LastLabel		TAG_USER+45
#define LACY_LabelTable		TAG_USER+98
#define LACY_AutoPageID		TAG_USER+103
#define LACY_TabKey		TAG_USER+118

/* FRACTION_KIND */
#define LAFR_IncrementerHook	TAG_USER+85

/* FRAME_KIND */
#define LAFR_InnerWidth		TAG_USER+9
#define LAFR_InnerHeight	TAG_USER+10
#define LAFR_DrawBox		TAG_USER+11
#define LAFR_RefreshHook	TAG_USER+117
#define LAFR_GenerateEvents	TAG_USER+155
#define LAFR_ResizeX		LALV_ResizeX
#define LAFR_ResizeY		LALV_ResizeY

/* GAUGE_KIND */
#define LAGA_Percent		TAG_USER+36
#define LAGA_InfoLength		TAG_USER+70
#define LAGA_InfoText		TAG_USER+71
#define LAGA_NoTicks		TAG_USER+143
#define LAGA_Discrete		TAG_USER+144
#define LAGA_Tenth		TAG_USER+144

/* IMAGE_KIND */
#define LAIM_Image		TAG_USER+181
#define LAIM_BitMap		TAG_USER+182
#define LAIM_BitMapLeft		TAG_USER+183
#define LAIM_BitMapTop		TAG_USER+184
#define LAIM_BitMapWidth	TAG_USER+185
#define LAIM_BitMapHeight	TAG_USER+186
#define LAIM_BitMapMask		TAG_USER+187

/* INTEGER_KIND */
#define LAIN_LastGadget		TAG_USER+28
#define LAIN_Min		TAG_USER+23
#define LAIN_Max		TAG_USER+24
#define LAIN_UseIncrementers	TAG_USER+57
#define LAIN_Incrementers	TAG_USER+57
#define LAIN_HistoryLines	TAG_USER+59
#define LAIN_HistoryHook	TAG_USER+80
#define LAIN_IncrementerHook	TAG_USER+85
#define LAIN_Activate		TAG_USER+148

/* LISTVIEW_KIND */
#define LALV_ExtraLabels	TAG_USER+26
#define LALV_Labels		TAG_USER+33
#define LALV_CursorKey		TAG_USER+35
#define LALV_Columns		TAG_USER+2
#define LALV_Lines		TAG_USER+1
#define LALV_Link		TAG_USER+7
#define LALV_FirstLabel		TAG_USER+44
#define LALV_LastLabel		TAG_USER+45
#define LALV_MaxGrowX		TAG_USER+77
#define LALV_MaxGrowY		TAG_USER+78
#define LALV_LabelTable		TAG_USER+98
#define LALV_LockSize		TAG_USER+106
#define LALV_ResizeX		TAG_USER+109
#define LALV_ResizeY		TAG_USER+110
#define LALV_MinChars		TAG_USER+111
#define LALV_MinLines		TAG_USER+112
#define LALV_FlushLabelLeft	TAG_USER+113
#define LALV_TextAttr		TAG_USER+138
#define LALV_AutoPageID		TAG_USER+103
#define LALV_Selected		TAG_USER+167
#define LALV_AdjustForString	TAG_USER+174

/* LEVEL_KIND */
#define LAVL_Min		GTSL_Min
#define LAVL_Max		GTSL_Max
#define LAVL_Level		GTSL_Level
#define LAVL_LevelFormat	GTSL_LevelFormat
#define LAVL_LevelPlace		GTSL_LevelPlace
#define LAVL_DispFunc		GTSL_DispFunc
#define LAVL_FullCheck		LASL_FullCheck
#define LAVL_Freedom		TAG_USER+177	/* (I)  New in V41 */
#define LAVL_Ticks		TAG_USER+178	/* (I)  New in V41 */
#define LAVL_NumTicks		TAG_USER+179	/* (IS) New in V41 */
#define LAVL_Lines		TAG_USER+1

/* MX_KIND */
#define LAMX_FirstLabel		TAG_USER+44
#define LAMX_LastLabel		TAG_USER+45
#define LAMX_LabelTable		TAG_USER+98
#define LAMX_TabKey		TAG_USER+118
#define LAMX_AutoPageID		TAG_USER+103

/* PALETTE_KIND */
#define LAPA_SmallPalette	TAG_USER+32
#define LAPA_Lines		TAG_USER+1
#define LAPA_UsePicker		TAG_USER+137
#define LAPA_Picker		TAG_USER+137

/* PASSWORD_KIND */
#define LAPW_String             GTST_String
#define LAPW_LastGadget		TAG_USER+28
#define LAPW_HistoryLines	TAG_USER+59
#define LAPW_HistoryHook	TAG_USER+80
#define LAPW_Activate		TAG_USER+148
#define LAPW_MaxChars		GTST_MaxChars

/* POPUP_KIND */
#define LAPU_FirstLabel		TAG_USER+44
#define LAPU_LastLabel		TAG_USER+45
#define LAPU_LabelTable		TAG_USER+98
#define LAPU_AutoPageID		TAG_USER+103
#define LAPU_TabKey		TAG_USER+118
#define LAPU_Labels		GTCY_Labels
#define LAPU_Active		GTCY_Active
#define LAPU_CentreActive	TAG_USER+163

/* SLIDER_KIND */
#define LASL_FullCheck		TAG_USER+22

/* SCROLLER_KIND */
#define LASC_Thin		TAG_USER+62
#define LASC_FullSize		TAG_USER+188

/* STRING_KIND */
#define LAST_LastGadget		TAG_USER+28
#define LAST_Link		TAG_USER+7
#define LAST_Picker		TAG_USER+5
#define LAST_UsePicker		TAG_USER+5
#define LAST_HistoryLines	TAG_USER+59
#define LAST_HistoryHook	TAG_USER+80
#define LAST_CursorPosition	TAG_USER+105
#define LAST_Activate		TAG_USER+148
#define LAST_ValidateHook	TAG_USER+165

/* TAB_KIND */
#define LATB_FirstLabel		TAG_USER+44
#define LATB_LastLabel		TAG_USER+45
#define LATB_LabelTable		TAG_USER+98
#define LATB_AutoPageID		TAG_USER+103
#define LATB_TabKey		TAG_USER+118
#define LATB_Labels		GTCY_Labels
#define LATB_Active		GTCY_Active
#define LATB_FullWidth		TAG_USER+149
#define LATB_FullSize		TAG_USER+149

/* TAPEDECK_KIND */
#define LATD_ButtonType		TAG_USER+86
#define LATD_Toggle		TAG_USER+87
#define LATD_Pressed		TAG_USER+88
#define LATD_Smaller		TAG_USER+89
#define LATD_Tick		TAG_USER+139

/* TEXT_KIND */
#define LATX_Picker		TAG_USER+5
#define LATX_UsePicker		TAG_USER+5
#define LATX_LockSize		TAG_USER+106

/* TEXTEDIT_KIND */
#define LATE_String		GTST_String
#define LATE_CursorPosition	STRINGA_BufferPos
#define LATE_Chars		LA_Chars
#define LATE_Lines		LABX_Rows
#define LATE_TextAttr		LALV_TextAttr

/* VERTICAL_KIND and HORIZONTAL_KIND */
#define LAGR_Spread		TAG_USER+6
#define LAGR_SameSize		TAG_USER+8
#define LAGR_LastAttributes	TAG_USER+46
#define LAGR_ActivePage		TAG_USER+58
#define LAGR_Frame		TAG_USER+104
#define LAGR_IndentX		TAG_USER+130
#define LAGR_IndentY		TAG_USER+134
#define LAGR_NoIndent		TAG_USER+146
#define LAGR_SameWidth		TAG_USER+150
#define LAGR_SameHeight		TAG_USER+151
#define LAGR_FrameGroup		TAG_USER+168
#define LAGR_AlignRight		TAG_USER+171

/* XBAR_KIND */
#define LAXB_FullSize		TAG_USER+50
#define LAXB_FullWidth		TAG_USER+50

/* Applicable for layout handle only */
#define LAHN_TextAttr		TAG_USER+41
#define LAHN_AutoActivate	TAG_USER+42
#define LAHN_LocaleHook		TAG_USER+4
#define LAHN_CloningPermitted	TAG_USER+61
#define LAHN_EditHook		TAG_USER+74
#define LAHN_ExactClone		TAG_USER+75
#define LAHN_MenuGlyphs		TAG_USER+76
#define LAHN_Parent		TAG_USER+83
#define LAHN_BlockParent	TAG_USER+84
#define LAHN_SimpleClone	TAG_USER+90
#define LAHN_ExitFlush		TAG_USER+108
#define LAHN_UserData		TAG_USER+116
#define LAHN_RawKeyFilter	TAG_USER+142
#define LAHN_DontPickShortcuts	TAG_USER+154
#define LAHN_NoKeys		TAG_USER+154
#define LAHN_PubScreen		TAG_USER+157
#define LAHN_PubScreenName	TAG_USER+158
#define LAHN_PubScreenFallBack	TAG_USER+159
#define LAHN_CloneScreenTitle	TAG_USER+175
#define LAHN_CloneScreenTitleID	TAG_USER+176
#define LAHN_TopGroupType	TAG_USER+190

/* Applicable for menus only. */
#define LAMN_FirstLabel		LABX_FirstLabel
#define LAMN_LastLabel		LABX_LastLabel
#define LAMN_LabelTable		TAG_USER+98
#define LAMN_TitleText		TAG_USER+17000
#define LAMN_TitleID		TAG_USER+17001
#define LAMN_ItemText		TAG_USER+17002
#define LAMN_ItemID		TAG_USER+17003
#define LAMN_SubText		TAG_USER+17004
#define LAMN_SubID		TAG_USER+17005
#define LAMN_KeyText		TAG_USER+17006
#define LAMN_KeyID		TAG_USER+17007
#define LAMN_CommandText	TAG_USER+17008
#define LAMN_CommandID		TAG_USER+17009
#define LAMN_MutualExclude	TAG_USER+17010
#define LAMN_UserData		TAG_USER+17011
#define LAMN_Disabled		TAG_USER+17012
#define LAMN_CheckIt		TAG_USER+17013
#define LAMN_Checked		TAG_USER+17014
#define LAMN_Toggle		TAG_USER+17015
#define LAMN_Code		TAG_USER+17016
#define LAMN_Qualifier		TAG_USER+17017
#define LAMN_Char		TAG_USER+17018
#define LAMN_ID			TAG_USER+17019
#define LAMN_AmigaGlyph		TAG_USER+17020
#define LAMN_CheckmarkGlyph	TAG_USER+17021
#define LAMN_Error		TAG_USER+17022
#define LAMN_Screen		TAG_USER+17023
#define LAMN_TextAttr		TAG_USER+17024
#define LAMN_LayoutHandle	TAG_USER+17025
#define LAMN_Handle		TAG_USER+17025
#define LAMN_ExtraSpace		TAG_USER+17026
#define LAMN_FullMenuNum	TAG_USER+160

/* Applicable for window only */
#define LAWN_Menu		TAG_USER+25
#define LAWN_UserPort		TAG_USER+47
#define LAWN_Left		TAG_USER+48
#define LAWN_Top		TAG_USER+49
#define LAWN_Zoom		TAG_USER+50
#define LAWN_MaxPen		TAG_USER+52
#define LAWN_BelowMouse		TAG_USER+53
#define LAWN_MoveToWindow	TAG_USER+54
#define LAWN_AutoRefresh	TAG_USER+55
#define LAWN_HelpHook		TAG_USER+73
#define LAWN_Parent		TAG_USER+81
#define LAWN_BlockParent	TAG_USER+82
#define LAWN_SmartZoom		TAG_USER+91
#define LAWN_Title		TAG_USER+92
#define LAWN_TitleText		TAG_USER+92
#define LAWN_Bounds		TAG_USER+93
#define LAWN_ExtraWidth		TAG_USER+94
#define LAWN_ExtraHeight	TAG_USER+95
#define LAWN_IDCMP		TAG_USER+96
#define LAWN_AlignWindow	TAG_USER+97
#define LAWN_TitleID		TAG_USER+99
#define LAWN_FlushLeft		TAG_USER+14000	/* NOTEZ-BIEN: TAG_USER+99 = WA_Dummy and can clash */
#define LAWN_FlushTop		TAG_USER+14001	/*             with Intuition!                      */
#define LAWN_Show		TAG_USER+14002
#define LAWN_MenuTemplate	TAG_USER+14003
#define LAWN_MenuTags		TAG_USER+14004
#define LAWN_NoInitialRefresh	TAG_USER+164
#define LAWN_LimitWidth		TAG_USER+165
#define LAWN_LimitHeight	TAG_USER+166
#define LAWN_UserData		TAG_USER+169

/* Private tags; do not use, or you'll run into trouble! */
#define LA_Private1		TAG_USER+100
#define LA_Private2		TAG_USER+101

/* Last tag item value used */
#define LAST_TAG		TAG_USER+190


/*****************************************************************************/


/* Identifies the absence of a link for a listview or a string gadget */
#define NIL_LINK (-2)


/*****************************************************************************/


/* String gadget type history hook support: you will either get
 * the following value passed as the message parameter to your
 * hook function, or a pointer to a null-terminated string you should
 * copy and create a Node from, which you should then add to the tail
 * of your history list. Place a pointer to your history list in the
 * Hook.h_Data entry.
 */
#define HISTORYHOOK_DiscardOldest (0)


/*****************************************************************************/


/* Refresh hook support: you will get the following structure
 * passed as the message and a pointer to the LayoutHandle as
 * the object.
 */
typedef struct RefreshMsg
{
	LONG ID;
	WORD Left;
	WORD Top;
	WORD Width;
	WORD Height;
} RefreshMsg;


/*****************************************************************************/


/* Incrementer hook support: you will get the current value
 * passed as the object and one of the following values as
 * the message. Return the number to be used.
 */
enum
{
	INCREMENTERMSG_Decrement = -1,	/* Decrement value */
	INCREMENTERMSG_Initial   =  0,	/* Initial value passed upon gadget creation */
	INCREMENTERMSG_Increment =  1	/* Increment value */
};


/*****************************************************************************/


/* Help key hook support: the hook will be called with a "struct IBox *"
 * as the object and a "struct HelpMsg *". The IBox describes the object
 * the mouse was positioned over, such as a button, a listview, etc.
 * The "ObjectID" will indicate the ID of the object the mouse was
 * positioned over. The ID will be -1 if no object was to be found.
 */
typedef struct HelpMsg
{
	struct LayoutHandle *	Handle;		/* Window layout handle */
	LONG			ObjectID;	/* ID of the object, -1 for full window */
} HelpMsg;


/*****************************************************************************/


/* This selects whether ticks should be placed next to the body of
 * a LEVEL_KIND object.
 */
enum
{
	TICKS_None  = 0,	/* No ticks please */
	TICKS_Left  = 1,	/* Place ticks left of the slider (FREEVERT only) */
	TICKS_Both  = 2		/* Place ticks on both sides of the slider */
};

#define TICKS_Above TICKS_Left	/* Place ticks above the slider (FREEHORIZ only) */


/*****************************************************************************/


/* The central data structure of the layout process. */
typedef struct LayoutHandle
{
	struct Screen *		Screen;
	struct DrawInfo *	DrawInfo;
	struct Window *		Window;
	APTR			VisualInfo;
	struct Image *		AmigaGlyph;
	struct Image *		CheckGlyph;
	APTR			UserData;	/* Requires gtlayout.library V9 */
	struct Menu *		Menu;		/* Requires gtlayout.library V13 */

	/* Hands off, private fields follow.... */
} LayoutHandle;


/*****************************************************************************/


/* Handy for LT_LevelWidth and LT_NewLevelWidth. Note: parameters must be passed
 * over the stack.
 */
typedef LONG (* DISPFUNC)(struct Gadget *gad,LONG value,...);


/*****************************************************************************/


/* Useful macros */
#define LT_GetString(Handle,Code)	((STRPTR)LT_GetAttributesA((Handle),(Code),NULL))

#define LAMN_Menu_UserData(m)		(*(APTR *)(((struct Menu *)(m)) + 1))
#define LAMN_Menu_ID(m)			(((ULONG *)(((struct Menu *)(m)) + 1))[1])

#define LAMN_Item_UserData(m)		(*(APTR *)(((struct MenuItem *)(m)) + 1))
#define LAMN_Item_ID(m)			(((ULONG *)(((struct MenuItem *)(m)) + 1))[1])


/*****************************************************************************/

#ifdef GTLAYOUT_OBSOLETE
/* Obsolete tags, don't use in new code */
#define LA_Lines		LABX_Rows
#define LA_Spread		LAGR_Spread
#define LA_SameSize		LAGR_SameSize
#define LA_FullCheck		LASL_FullCheck
#define LA_ExtraLabels		LALV_ExtraLabels
#define LA_LastGadget		TAG_USER+28
#define LA_SmallPalette		LAPA_SmallPalette
#define LA_Labels		LALV_Labels
#define LA_Picker		LATX_Picker
#define LA_DrawBox		LAFR_DrawBox
#define LA_FirstLabel		LABX_FirstLabel
#define LA_LastLabel		LABX_LastLabel
#define LA_LabelTable		LABX_LabelTable
#define LA_Min			TAG_USER+23
#define LA_Max			TAG_USER+24
#define LA_Link			LALV_Link
#define LA_Menu			LAWN_Menu
#define LA_HistoryLines		LAST_HistoryLines
#define LA_HistoryHook		LAST_HistoryHook
#define LA_ReturnKey		LABT_ReturnKey
#define LA_ExtraFat		LABT_ExtraFat
#define LA_CursorKey		LALV_CursorKey
#define STORE_BYTE		TAG_USER+63
#define STORE_UBYTE		TAG_USER+64
#define STORE_WORD		TAG_USER+65
#define STORE_BOOL		TAG_USER+65
#define STORE_UWORD		TAG_USER+66
#define STORE_LONG		TAG_USER+67
#define STORE_ULONG		TAG_USER+68
#define STORE_STRPTR		TAG_USER+69
#define LAHN_Font		TAG_USER+41
#define LH_Font			TAG_USER+41
#define LH_AutoActivate		TAG_USER+42
#define LH_LocaleHook		TAG_USER+4
#define LH_CloningPermitted	TAG_USER+61
#define LH_EditHook		TAG_USER+74
#define LH_ExactClone		TAG_USER+75
#define LH_MenuGlyphs		TAG_USER+76
#define LH_Parent		TAG_USER+83
#define LH_BlockParent		TAG_USER+84
#define LH_SimpleClone		TAG_USER+90
#define LH_ExitFlush		TAG_USER+108
#define LH_UserData		TAG_USER+116
#define LH_RawKeyFilter		TAG_USER+142


/* Obsolete defines, don't use in new code */
#define PLACE_LEFT		PLACE_Left
#define PLACE_RIGHT		PLACE_Right
#define PLACE_ABOVE		PLACE_Above
#define PLACE_IN		PLACE_In
#define PLACE_BELOW		PLACE_Below

#define ALIGNTEXT_LEFT		ALIGNTEXT_Left
#define ALIGNTEXT_CENTERED	ALIGNTEXT_Centered
#define ALIGNTEXT_RIGHT		ALIGNTEXT_Right
#define ALIGNTEXT_PAD		ALIGNTEXT_Pad

#define TDBT_BACKWARD		TDBT_Backward
#define TDBT_FORWARD		TDBT_Forward
#define TDBT_PREVIOUS		TDBT_Previous
#define TDBT_NEXT		TDBT_Next
#define TDBT_STOP		TDBT_Stop
#define TDBT_PAUSE		TDBT_Pause
#define TDBT_RECORD		TDBT_Record
#define TDBT_REWIND		TDBT_Rewind
#define TDBT_EJECT		TDBT_Eject
#define TDBT_PLAY		TDBT_Play

#define ALIGNF_RIGHT		ALIGNF_Right
#define ALIGNF_LEFT		ALIGNF_Left
#define ALIGNF_TOP		ALIGNF_Top
#define ALIGNF_BOTTOM		ALIGNF_Bottom
#define ALIGNF_EXTRA_RIGHT	ALIGNF_ExtraRight
#define ALIGNF_EXTRA_LEFT	ALIGNF_ExtraLeft
#define ALIGNF_EXTRA_TOP	ALIGNF_ExtraTop
#define ALIGNF_EXTRA_BOTTOM	ALIGNF_ExtraBottom

#define HISTORYHOOK_DISCARD_OLDEST	HISTORYHOOK_DiscardOldest

#define INCREMENTERMSG_DECREMENT	INCREMENTERMSG_Decrement
#define INCREMENTERMSG_INITIAL		INCREMENTERMSG_Initial
#define INCREMENTERMSG_INCREMENT	INCREMENTERMSG_Increment

/* Obsolete routines redone as macros. */
#define LT_GetDrawInfo(Handle)		((Handle != NULL) ? ((Handle)->DrawInfo)	: NULL)
#define LT_GetVisualInfo(Handle)	((Handle != NULL) ? ((Handle)->VisualInfo)	: NULL)
#define LT_GetScreen(Handle)		((Handle != NULL) ? ((Handle)->Screen)		: NULL)
#define LT_SetAutoActivate(Handle,Mode)	LT_SetAttributes(Handle,0,LH_AutoActivate,Mode,TAG_DONE)
#endif	/* GTLAYOUT_OBSOLETE */

/*****************************************************************************/


#endif	/* _GTLAYOUT_H */
