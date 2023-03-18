/* Automatically generated header (sfdc 1.11)! Do not edit! */

#ifndef _INLINE_GTLAYOUT_H
#define _INLINE_GTLAYOUT_H

#ifndef _SFDC_VARARG_DEFINED
#define _SFDC_VARARG_DEFINED
#ifdef __HAVE_IPTR_ATTR__
typedef APTR _sfdc_vararg __attribute__((iptr));
#else
typedef ULONG _sfdc_vararg;
#endif /* __HAVE_IPTR_ATTR__ */
#endif /* _SFDC_VARARG_DEFINED */

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef GTLAYOUT_BASE_NAME
#define GTLAYOUT_BASE_NAME GTLayoutBase
#endif /* !GTLAYOUT_BASE_NAME */

#define LT_LevelWidth(___handle, ___levelFormat, ___dispFunc, ___min, ___max, ___maxWidth, ___maxLen, ___fullCheck) \
      LP8NRA5(0x1e, LT_LevelWidth , struct LayoutHandle *, ___handle, a0, STRPTR, ___levelFormat, a1, APTR, ___dispFunc, a2, LONG, ___min, d0, LONG, ___max, d1, LONG *, ___maxWidth, a3, LONG *, ___maxLen, d7, LONG, ___fullCheck, d2,\
      , GTLAYOUT_BASE_NAME)

#define LT_DeleteHandle(___handle) \
      LP1NR(0x24, LT_DeleteHandle , struct LayoutHandle *, ___handle, a0,\
      , GTLAYOUT_BASE_NAME)

#define LT_CreateHandle(___screen, ___font) \
      LP2(0x2a, struct LayoutHandle *, LT_CreateHandle , struct Screen *, ___screen, a0, struct TextAttr *, ___font, a1,\
      , GTLAYOUT_BASE_NAME)

#define LT_CreateHandleTagList(___screen, ___tagList) \
      LP2(0x30, struct LayoutHandle *, LT_CreateHandleTagList , struct Screen *, ___screen, a0, struct TagItem *, ___tagList, a1,\
      , GTLAYOUT_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define LT_CreateHandleTags(___screen, ___dummy, ...) \
    ({_sfdc_vararg _tags[] = { ___dummy, __VA_ARGS__ }; LT_CreateHandleTagList((___screen), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define LT_Rebuild(___handle, ___bounds, ___extraWidth, ___extraHeight, ___clear) \
      LP5(0x36, BOOL, LT_Rebuild , struct LayoutHandle *, ___handle, a0, struct IBox *, ___bounds, a1, LONG, ___extraWidth, a2, LONG, ___extraHeight, d0, LONG, ___clear, d1,\
      , GTLAYOUT_BASE_NAME)

#define LT_HandleInput(___handle, ___msgQualifier, ___msgClass, ___msgCode, ___msgGadget) \
      LP5NR(0x3c, LT_HandleInput , struct LayoutHandle *, ___handle, a0, ULONG, ___msgQualifier, d0, ULONG *, ___msgClass, a1, UWORD *, ___msgCode, a2, struct Gadget **, ___msgGadget, a3,\
      , GTLAYOUT_BASE_NAME)

#define LT_BeginRefresh(___handle) \
      LP1NR(0x42, LT_BeginRefresh , struct LayoutHandle *, ___handle, a0,\
      , GTLAYOUT_BASE_NAME)

#define LT_EndRefresh(___handle, ___complete) \
      LP2NR(0x48, LT_EndRefresh , struct LayoutHandle *, ___handle, a0, LONG, ___complete, d0,\
      , GTLAYOUT_BASE_NAME)

#define LT_GetAttributesA(___handle, ___id, ___tagList) \
      LP3(0x4e, LONG, LT_GetAttributesA , struct LayoutHandle *, ___handle, a0, LONG, ___id, d0, struct TagItem *, ___tagList, a1,\
      , GTLAYOUT_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define LT_GetAttributes(___handle, ___id, ___dummy, ...) \
    ({_sfdc_vararg _tags[] = { ___dummy, __VA_ARGS__ }; LT_GetAttributesA((___handle), (___id), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define LT_SetAttributesA(___handle, ___id, ___tagList) \
      LP3NR(0x54, LT_SetAttributesA , struct LayoutHandle *, ___handle, a0, LONG, ___id, d0, struct TagItem *, ___tagList, a1,\
      , GTLAYOUT_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define LT_SetAttributes(___handle, ___id, ___dummy, ...) \
    ({_sfdc_vararg _tags[] = { ___dummy, __VA_ARGS__ }; LT_SetAttributesA((___handle), (___id), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define LT_AddA(___handle, ___type, ___label, ___id, ___tagList) \
      LP5NR(0x5a, LT_AddA , struct LayoutHandle *, ___handle, a0, LONG, ___type, d0, STRPTR, ___label, d1, LONG, ___id, d2, struct TagItem *, ___tagList, a1,\
      , GTLAYOUT_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define LT_Add(___handle, ___type, ___label, ___id, ___dummy, ...) \
    ({_sfdc_vararg _tags[] = { ___dummy, __VA_ARGS__ }; LT_AddA((___handle), (___type), (___label), (___id), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define LT_NewA(___handle, ___tagList) \
      LP2NR(0x60, LT_NewA , struct LayoutHandle *, ___handle, a0, struct TagItem *, ___tagList, a1,\
      , GTLAYOUT_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define LT_New(___handle, ___dummy, ...) \
    ({_sfdc_vararg _tags[] = { ___dummy, __VA_ARGS__ }; LT_NewA((___handle), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define LT_EndGroup(___handle) \
      LP1NR(0x66, LT_EndGroup , struct LayoutHandle *, ___handle, a0,\
      , GTLAYOUT_BASE_NAME)

#define LT_LayoutA(___handle, ___title, ___bounds, ___extraWidth, ___extraHeight, ___idcmp, ___align, ___tagParams) \
      LP8(0x6c, struct Window *, LT_LayoutA , struct LayoutHandle *, ___handle, a0, STRPTR, ___title, a1, struct IBox *, ___bounds, a2, LONG, ___extraWidth, d0, LONG, ___extraHeight, d1, ULONG, ___idcmp, d2, LONG, ___align, d3, struct TagItem *, ___tagParams, a3,\
      , GTLAYOUT_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define LT_Layout(___handle, ___title, ___bounds, ___extraWidth, ___extraHeight, ___idcmp, ___align, ___dummy, ...) \
    ({_sfdc_vararg _tags[] = { ___dummy, __VA_ARGS__ }; LT_LayoutA((___handle), (___title), (___bounds), (___extraWidth), (___extraHeight), (___idcmp), (___align), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define LT_LayoutMenusA(___handle, ___menuTemplate, ___tagParams) \
      LP3(0x72, struct Menu *, LT_LayoutMenusA , struct LayoutHandle *, ___handle, a0, struct NewMenu *, ___menuTemplate, a1, struct TagItem *, ___tagParams, a2,\
      , GTLAYOUT_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define LT_LayoutMenus(___handle, ___menuTemplate, ___dummy, ...) \
    ({_sfdc_vararg _tags[] = { ___dummy, __VA_ARGS__ }; LT_LayoutMenusA((___handle), (___menuTemplate), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define LT_LabelWidth(___handle, ___label) \
      LP2(0x8a, LONG, LT_LabelWidth , struct LayoutHandle *, ___handle, a0, STRPTR, ___label, a1,\
      , GTLAYOUT_BASE_NAME)

#define LT_LabelChars(___handle, ___label) \
      LP2(0x90, LONG, LT_LabelChars , struct LayoutHandle *, ___handle, a0, STRPTR, ___label, a1,\
      , GTLAYOUT_BASE_NAME)

#define LT_LockWindow(___window) \
      LP1NR(0x96, LT_LockWindow , struct Window *, ___window, a0,\
      , GTLAYOUT_BASE_NAME)

#define LT_UnlockWindow(___window) \
      LP1NR(0x9c, LT_UnlockWindow , struct Window *, ___window, a0,\
      , GTLAYOUT_BASE_NAME)

#define LT_DeleteWindowLock(___window) \
      LP1NR(0xa2, LT_DeleteWindowLock , struct Window *, ___window, a0,\
      , GTLAYOUT_BASE_NAME)

#define LT_ShowWindow(___handle, ___activate) \
      LP2NR(0xa8, LT_ShowWindow , struct LayoutHandle *, ___handle, a0, LONG, ___activate, a1,\
      , GTLAYOUT_BASE_NAME)

#define LT_Activate(___handle, ___id) \
      LP2NR(0xae, LT_Activate , struct LayoutHandle *, ___handle, a0, LONG, ___id, d0,\
      , GTLAYOUT_BASE_NAME)

#define LT_PressButton(___handle, ___id) \
      LP2(0xb4, BOOL, LT_PressButton , struct LayoutHandle *, ___handle, a0, LONG, ___id, d0,\
      , GTLAYOUT_BASE_NAME)

#define LT_GetCode(___msgQualifier, ___msgClass, ___msgCode, ___msgGadget) \
      LP4(0xba, LONG, LT_GetCode , ULONG, ___msgQualifier, d0, ULONG, ___msgClass, d1, ULONG, ___msgCode, d2, struct Gadget *, ___msgGadget, a0,\
      , GTLAYOUT_BASE_NAME)

#define LT_GetIMsg(___handle) \
      LP1(0xc0, struct IntuiMessage *, LT_GetIMsg , struct LayoutHandle *, ___handle, a0,\
      , GTLAYOUT_BASE_NAME)

#define LT_ReplyIMsg(___msg) \
      LP1NR(0xc6, LT_ReplyIMsg , struct IntuiMessage *, ___msg, a0,\
      , GTLAYOUT_BASE_NAME)

#define LT_BuildA(___handle, ___tagParams) \
      LP2(0xcc, struct Window *, LT_BuildA , struct LayoutHandle *, ___handle, a0, struct TagItem *, ___tagParams, a1,\
      , GTLAYOUT_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define LT_Build(___handle, ___dummy, ...) \
    ({_sfdc_vararg _tags[] = { ___dummy, __VA_ARGS__ }; LT_BuildA((___handle), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define LT_RebuildTagList(___handle, ___clear, ___tags) \
      LP3(0xd2, BOOL, LT_RebuildTagList , struct LayoutHandle *, ___handle, a0, LONG, ___clear, d0, struct TagItem *, ___tags, a1,\
      , GTLAYOUT_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define LT_RebuildTags(___handle, ___clear, ___dummy, ...) \
    ({_sfdc_vararg _tags[] = { ___dummy, __VA_ARGS__ }; LT_RebuildTagList((___handle), (___clear), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define LT_UpdateStrings(___handle) \
      LP1NR(0xd8, LT_UpdateStrings , struct LayoutHandle *, ___handle, a0,\
      , GTLAYOUT_BASE_NAME)

#define LT_DisposeMenu(___menu) \
      LP1NR(0xde, LT_DisposeMenu , struct Menu *, ___menu, a0,\
      , GTLAYOUT_BASE_NAME)

#define LT_NewMenuTemplate(___screen, ___textAttr, ___amigaGlyph, ___checkGlyph, ___error, ___menuTemplate) \
      LP6(0xe4, struct Menu *, LT_NewMenuTemplate , struct Screen *, ___screen, a0, struct TextAttr *, ___textAttr, a1, struct Image *, ___amigaGlyph, a2, struct Image *, ___checkGlyph, a3, LONG *, ___error, d0, struct NewMenu *, ___menuTemplate, d1,\
      , GTLAYOUT_BASE_NAME)

#define LT_NewMenuTagList(___tagList) \
      LP1(0xea, struct Menu *, LT_NewMenuTagList , struct TagItem *, ___tagList, a0,\
      , GTLAYOUT_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define LT_MenuControlTagList(___window, ___intuitionMenu, ____tag1, ...) \
    ({_sfdc_vararg _tags[] = { ____tag1, __VA_ARGS__ }; LT_NewMenuTagList((___window), (___intuitionMenu), (___tags), ); })
#endif /* !NO_INLINE_STDARG */

#ifndef NO_INLINE_STDARG
#define LT_MenuControlTags(___window, ___intuitionMenu, ___dummy, ...) \
    ({_sfdc_vararg _tags[] = { ___dummy, __VA_ARGS__ }; LT_NewMenuTagList((___window), (___intuitionMenu), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define LT_GetMenuItem(___menu, ___id) \
      LP2(0xf0, struct MenuItem *, LT_GetMenuItem , struct Menu *, ___menu, a0, ULONG, ___id, d0,\
      , GTLAYOUT_BASE_NAME)

#define LT_FindMenuCommand(___menu, ___msgCode, ___msgQualifier, ___msgGadget) \
      LP4(0xf6, struct MenuItem *, LT_FindMenuCommand , struct Menu *, ___menu, a0, ULONG, ___msgCode, d0, ULONG, ___msgQualifier, d1, struct Gadget *, ___msgGadget, a1,\
      , GTLAYOUT_BASE_NAME)

#define LT_NewLevelWidth(___handle, ___levelFormat, ___dispFunc, ___min, ___max, ___maxWidth, ___maxLen, ___fullCheck) \
      LP8NR(0xfc, LT_NewLevelWidth , struct LayoutHandle *, ___handle, a0, STRPTR, ___levelFormat, a1, APTR, ___dispFunc, a2, LONG, ___min, d0, LONG, ___max, d1, LONG *, ___maxWidth, a3, LONG *, ___maxLen, d3, LONG, ___fullCheck, d2,\
      , GTLAYOUT_BASE_NAME)

#define LT_Refresh(___handle) \
      LP1NR(0x102, LT_Refresh , struct LayoutHandle *, ___handle, a0,\
      , GTLAYOUT_BASE_NAME)

#define LT_CatchUpRefresh(___handle) \
      LP1NR(0x108, LT_CatchUpRefresh , struct LayoutHandle *, ___handle, a0,\
      , GTLAYOUT_BASE_NAME)

#define LT_GetWindowUserData(___window, ___defaultValue) \
      LP2(0x10e, APTR, LT_GetWindowUserData , struct Window *, ___window, a0, APTR, ___defaultValue, a1,\
      , GTLAYOUT_BASE_NAME)

#define LT_Redraw(___handle, ___id) \
      LP2NR(0x114, LT_Redraw , struct LayoutHandle *, ___handle, a0, LONG, ___id, d0,\
      , GTLAYOUT_BASE_NAME)

#endif /* !_INLINE_GTLAYOUT_H */
