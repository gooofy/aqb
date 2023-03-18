#ifndef  CLIB_GTLAYOUT_PROTOS_H
#define  CLIB_GTLAYOUT_PROTOS_H

/*
**	$VER: gtlayout_protos.h 47.1 (11.10.1999)
**
**	C prototypes. For use with 32 bit integers only.
**
**	Copyright © 1993-1999 by Olaf `Olsen' Barthel
**		Freely distributable.
*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef  EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef  INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif
VOID LT_LevelWidth( struct LayoutHandle *handle, STRPTR levelFormat, APTR dispFunc, LONG min, LONG max, LONG *maxWidth, LONG *maxLen, LONG fullCheck );
VOID LT_DeleteHandle( struct LayoutHandle *handle );
struct LayoutHandle *LT_CreateHandle( struct Screen *screen, struct TextAttr *font );
struct LayoutHandle *LT_CreateHandleTagList( struct Screen *screen, struct TagItem *tagList );
struct LayoutHandle *LT_CreateHandleTags( struct Screen *screen, ... );
BOOL LT_Rebuild( struct LayoutHandle *handle, struct IBox *bounds, LONG extraWidth, LONG extraHeight, LONG clear );
VOID LT_HandleInput( struct LayoutHandle *handle, ULONG msgQualifier, ULONG *msgClass, UWORD *msgCode, struct Gadget **msgGadget );
VOID LT_BeginRefresh( struct LayoutHandle *handle );
VOID LT_EndRefresh( struct LayoutHandle *handle, LONG complete );
LONG LT_GetAttributesA( struct LayoutHandle *handle, LONG id, struct TagItem *tagList );
LONG LT_GetAttributes( struct LayoutHandle *handle, LONG id, ... );
VOID LT_SetAttributesA( struct LayoutHandle *handle, LONG id, struct TagItem *tagList );
VOID LT_SetAttributes( struct LayoutHandle *handle, LONG id, ... );
VOID LT_AddA( struct LayoutHandle *handle, LONG type, STRPTR label, LONG id, struct TagItem *tagList );
VOID LT_Add( struct LayoutHandle *handle, LONG type, STRPTR label, LONG id, ... );
VOID LT_NewA( struct LayoutHandle *handle, struct TagItem *tagList );
VOID LT_New( struct LayoutHandle *handle, ... );
VOID LT_EndGroup( struct LayoutHandle *handle );
struct Window *LT_LayoutA( struct LayoutHandle *handle, STRPTR title, struct IBox *bounds, LONG extraWidth, LONG extraHeight, ULONG idcmp, LONG align, struct TagItem *tagParams );
struct Window *LT_Layout( struct LayoutHandle *handle, STRPTR title, struct IBox *bounds, LONG extraWidth, LONG extraHeight, ULONG idcmp, LONG align, ... );
struct Menu *LT_LayoutMenusA( struct LayoutHandle *handle, struct NewMenu *menuTemplate, struct TagItem *tagParams );
struct Menu *LT_LayoutMenus( struct LayoutHandle *handle, struct NewMenu *menuTemplate, ... );
LONG LT_LabelWidth( struct LayoutHandle *handle, STRPTR label );
LONG LT_LabelChars( struct LayoutHandle *handle, STRPTR label );
VOID LT_LockWindow( struct Window *window );
VOID LT_UnlockWindow( struct Window *window );
VOID LT_DeleteWindowLock( struct Window *window );
VOID LT_ShowWindow( struct LayoutHandle *handle, LONG activate );
VOID LT_Activate( struct LayoutHandle *handle, LONG id );
BOOL LT_PressButton( struct LayoutHandle *handle, LONG id );
LONG LT_GetCode( ULONG msgQualifier, ULONG msgClass, ULONG msgCode, struct Gadget *msgGadget );
/*--- Added in v1.78 --------------------------------------------------*/
struct IntuiMessage *LT_GetIMsg( struct LayoutHandle *handle );
VOID LT_ReplyIMsg( struct IntuiMessage *msg );
/*--- Added in v3.0 ---------------------------------------------------*/
struct Window *LT_BuildA( struct LayoutHandle *handle, struct TagItem *tagParams );
struct Window *LT_Build( struct LayoutHandle *handle, ... );
BOOL LT_RebuildTagList( struct LayoutHandle *handle, LONG clear, struct TagItem *tags );
BOOL LT_RebuildTags( struct LayoutHandle *handle, LONG clear, ... );
/*--- Added in v9.0 ---------------------------------------------------*/
VOID LT_UpdateStrings( struct LayoutHandle *handle );
/*--- Added in v11.0 ---------------------------------------------------*/
VOID LT_DisposeMenu( struct Menu *menu );
struct Menu *LT_NewMenuTemplate( struct Screen *screen, struct TextAttr *textAttr, struct Image *amigaGlyph, struct Image *checkGlyph, LONG *error, struct NewMenu *menuTemplate );
struct Menu *LT_NewMenuTagList( struct TagItem *tagList );
struct Menu *LT_NewMenuTags( Tag firstTag, ... );
VOID LT_MenuControlTagList( struct Window *window, struct Menu *intuitionMenu, struct TagItem *tags );
VOID LT_MenuControlTags( struct Window *window, struct Menu *intuitionMenu, ... );
struct MenuItem *LT_GetMenuItem( struct Menu *menu, ULONG id );
struct MenuItem *LT_FindMenuCommand( struct Menu *menu, ULONG msgCode, ULONG msgQualifier, struct Gadget *msgGadget );
/*--- Added in v14.0 ---------------------------------------------------*/
VOID LT_NewLevelWidth( struct LayoutHandle *handle, STRPTR levelFormat, APTR dispFunc, LONG min, LONG max, LONG *maxWidth, LONG *maxLen, LONG fullCheck );
/*--- Added in v31.0 ---------------------------------------------------*/
VOID LT_Refresh( struct LayoutHandle *handle );
/*--- Added in v34.0 ---------------------------------------------------*/
VOID LT_CatchUpRefresh( struct LayoutHandle *handle );
/*--- Added in v39.0 ---------------------------------------------------*/
APTR LT_GetWindowUserData( struct Window *window, APTR defaultValue );
/*--- Added in v47.0 ---------------------------------------------------*/
VOID LT_Redraw( struct LayoutHandle *handle, LONG id );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif   /* CLIB_GTLAYOUT_PROTOS_H */
