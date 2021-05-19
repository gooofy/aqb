#ifndef LIBRARIES_REQTOOLS_H
#define LIBRARIES_REQTOOLS_H
/*
**	$Filename: libraries/reqtools.h $
**	$Release: 1.0 $
**
**	(C) Copyright 1991 Nico François
**	    All Rights Reserved
*/

#ifndef	EXEC_TYPES_H
#include <exec/types.h>
#endif	/* EXEC_TYPES_H */

#ifndef	EXEC_LISTS_H
#include <exec/lists.h>
#endif	/* EXEC_LISTS_H */

#ifndef	EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif	/* EXEC_LIBRARIES_H */

#ifndef	GRAPHICS_TEXT_H
#include <graphics/text.h>
#endif	/* GRAPHICS_TEXT_H */

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif	/* UTILITY_TAGITEM_H */

#define	REQTOOLSNAME		 "reqtools.library"
#define	REQTOOLSVERSION		 37L

struct ReqToolsBase {
   struct Library LibNode;
   UBYTE Flags;
   UBYTE pad[3];
   BPTR SegList;
   /* The following library bases may be read and used by your program */
   struct IntuitionBase *IntuitionBase;
   struct GfxBase *GfxBase;
   struct DosLibrary *DOSBase;
   /* Next two library bases are only (and always) valid on Kickstart 2.0!
      (1.3 version of reqtools also initializes these when run on 2.0) */
   struct Library *GadToolsBase;
   struct Library *UtilityBase;
   };

/* types of requesters, for rtAllocRequestA() */
#define RT_FILEREQ		 0L
#define RT_REQINFO		 1L
#define RT_FONTREQ		 2L

/***********************
*                      *
*    File requester    *
*                      *
***********************/

/* structure _MUST_ be allocated with rtAllocRequest() */

struct rtFileRequester {
   ULONG ReqPos;
   UWORD LeftOffset;
   UWORD TopOffset;
   ULONG Flags;
   struct Hook *Hook;
   char *Dir;                /* READ ONLY! Change with rtChangeReqAttrA()! */
   char *MatchPat;           /* READ ONLY! Change with rtChangeReqAttrA()! */
   struct TextFont *DefaultFont;
   ULONG WaitPointer;
   /* Lots of private data follows! HANDS OFF :-) */
   };

/* returned by rtFileRequestA() if multiselect is enabled,
   free list with rtFreeFileList() */

struct rtFileList {
   struct rtFileList *Next;
   ULONG StrLen;             /* -1 for directories */
   char *Name;
   };

/***********************
*                      *
*    Font requester    *
*                      *
***********************/

/* structure _MUST_ be allocated with rtAllocRequest() */

struct rtFontRequester {
   ULONG ReqPos;
   UWORD LeftOffset;
   UWORD TopOffset;
   ULONG Flags;
   struct Hook *Hook;
   struct TextAttr Attr;		/* READ ONLY! */
   struct TextFont *DefaultFont;
   ULONG WaitPointer;
   /* Lots of private data follows! HANDS OFF :-) */
   };

/***********************
*                      *
*    Requester Info    *
*                      *
***********************/

/* for rtEZRequestA(), rtGetLongA(), rtGetStringA() and rtPaletteRequestA(),
   _MUST_ be allocated with rtAllocRequest() */

struct rtReqInfo {
   ULONG ReqPos;
   UWORD LeftOffset;
   UWORD TopOffset;
   ULONG Width;                   /* not for rtEZRequestA() */
   char *ReqTitle;                /* currently only for rtEZRequestA() */
   ULONG Flags;                   /* only for rtEZRequestA() */
   struct TextFont *DefaultFont;  /* currently only for rtPaletteRequestA() */
   ULONG WaitPointer;
   /* structure may be extended in future */
   };

/***********************
*                      *
*     Handler Info     *
*                      *
***********************/

/* for rtReqHandlerA(), will be allocated for you when you use
   the RT_ReqHandler tag, never try to allocate this yourself! */

struct rtHandlerInfo {
   ULONG private1;
   ULONG WaitMask;
   ULONG DoNotWait;
   /* Private data follows, HANDS OFF :-) */
   };

/* possible return codes from rtReqHandlerA() */

#define CALL_HANDLER		 (ULONG)0x80000000


/*************************************
*                                    *
*                TAGS                *
*                                    *
*************************************/

#define RT_TagBase		 TAG_USER

/*** tags understood by most requester functions ***
*/
/* optional pointer to window */
#define RT_Window		 (RT_TagBase+1)
/* idcmp flags requester should abort on (useful for IDCMP_DISKINSERTED) */
#define RT_IDCMPFlags		 (RT_TagBase+2)
/* position of requester window (see below) - default REQPOS_POINTER */
#define RT_ReqPos		 (RT_TagBase+3)
/* signal mask to wait for abort signal */
#define RT_LeftOffset		 (RT_TagBase+4)
/* topedge offset of requester relative to position specified by RT_ReqPos */
#define RT_TopOffset		 (RT_TagBase+5)
/* name of public screen to put requester on (Kickstart 2.0 only!) */
#define RT_PubScrName		 (RT_TagBase+6)
/* address of screen to put requester on */
#define RT_Screen		 (RT_TagBase+7)
/* tagdata must hold the address of (!) an APTR variable */
#define RT_ReqHandler		 (RT_TagBase+8)
/* font to use when screen font is rejected, _MUST_ be fixed-width font!
   (struct TextFont *, not struct TextAttr *!)
   - default GfxBase->DefaultFont */
#define RT_DefaultFont		 (RT_TagBase+9)
/* boolean to set the standard wait pointer in window - default FALSE */
#define RT_WaitPointer		 (RT_TagBase+10)

/*** tags specific to rtEZRequestA ***
*/
/* title of requester window - default "Request" or "Information" */
#define RTEZ_ReqTitle		 (RT_TagBase+20)
/* (RT_TagBase+21) reserved
 various flags (see below) */
#define RTEZ_Flags		 (RT_TagBase+22)
/* default response (activated by pressing RETURN) - default TRUE */
#define RTEZ_DefaultResponse	 (RT_TagBase+23)

/*** tags specific to rtGetLongA ***
*/
/* minimum allowed value - default MININT */
#define RTGL_Min		 (RT_TagBase+30)
/* maximum allowed value - default MAXINT */
#define RTGL_Max		 (RT_TagBase+31)
/* suggested width of requester window (in pixels) */
#define RTGL_Width		 (RT_TagBase+32)
/* boolean to show the default value - default TRUE */
#define RTGL_ShowDefault	 (RT_TagBase+33)

/*** tags specific to rtGetStringA ***
*/
/* suggested width of requester window (in pixels) */
#define RTGS_Width		 RTGL_Width
/* allow empty string to be accepted - default FALSE */
#define RTGS_AllowEmpty		 (RT_TagBase+80)

/*** tags specific to rtFileRequestA ***
*/
/* various flags (see below) */
#define RTFI_Flags		 (RT_TagBase+40)
/* suggested height of file requester */
#define RTFI_Height		 (RT_TagBase+41)
/* replacement text for 'Ok' gadget (max 6 chars) */
#define RTFI_OkText		 (RT_TagBase+42)

/*** tags specific to rtFontRequestA ***
*/
/* various flags (see below) */
#define RTFO_Flags		 RTFI_Flags
/* suggested height of font requester */
#define RTFO_Height		 RTFI_Height
/* replacement text for 'Ok' gadget (max 6 chars) */
#define RTFO_OkText		 RTFI_OkText
/* suggested height of font sample display - default 24 */
#define RTFO_SampleHeight	 (RT_TagBase+60)
/* minimum height of font displayed */
#define RTFO_MinHeight		 (RT_TagBase+61)
/* maximum height of font displayed */
#define RTFO_MaxHeight		 (RT_TagBase+62)
/* [(RT_TagBase+63) to (RT_TagBase+66) used below] */

/*** tags for rtChangeReqAttrA ***
*/
/* file requester - set directory */
#define RTFI_Dir		 (RT_TagBase+50)
/* file requester - set wildcard pattern */
#define RTFI_MatchPat		 (RT_TagBase+51)
/* file requester - add a file or directory to the buffer */
#define RTFI_AddEntry		 (RT_TagBase+52)
/* file requester - remove a file or directory from the buffer */
#define RTFI_RemoveEntry	 (RT_TagBase+53)
/* font requester - set font name of selected font */
#define RTFO_FontName		 (RT_TagBase+63)
/* font requester - set font size */
#define RTFO_FontHeight		 (RT_TagBase+64)
/* font requester - set font style */
#define RTFO_FontStyle		 (RT_TagBase+65)
/* font requester - set font flags */
#define RTFO_FontFlags		 (RT_TagBase+66)

/*** tags for rtPaletteRequestA ***
*/
/* initially selected color - default 1 */
#define RTPA_Color		 (RT_TagBase+70)

/*** tags for rtReqHandlerA ***
*/
/* end requester by software control, set tagdata to REQ_CANCEL, REQ_OK or
   in case of rtEZRequest to the return value */
#define RTRH_EndRequest		 (RT_TagBase+60)

/*** tags for rtAllocRequestA ***/
/* no tags defined yet */


/************
* RT_ReqPos *
************/
#define REQPOS_POINTER		 0L
#define REQPOS_CENTERWIN	 1L
#define REQPOS_CENTERSCR	 2L
#define REQPOS_TOPLEFTWIN	 3L
#define REQPOS_TOPLEFTSCR	 4L

/******************
* RTRH_EndRequest *
******************/
#define REQ_CANCEL		 0L
#define REQ_OK			 1L

/***************************************
* flags for RTFI_Flags and RTFO_Flags  *
* or filereq->Flags and fontreq->Flags *
***************************************/
#define FREQB_NOBUFFER		 2L
#define FREQF_NOBUFFER		 (1L<<FREQB_NOBUFFER)
#define FREQB_DOWILDFUNC	 11L
#define FREQF_DOWILDFUNC	 (1L<<FREQB_DOWILDFUNC)

/*****************************************
* flags for RTFI_Flags or filereq->Flags *
*****************************************/
#define FREQB_MULTISELECT	 0L
#define FREQF_MULTISELECT	 (1L<<FREQB_MULTISELECT)
#define FREQB_SAVE		 1L
#define FREQF_SAVE		 (1L<<FREQB_SAVE)
#define FREQB_NOFILES		 3L
#define FREQF_NOFILES		 (1L<<FREQB_NOFILES)
#define FREQB_PATGAD		 4L
#define FREQF_PATGAD		 (1L<<FREQB_PATGAD)
#define FREQB_SELECTDIRS	 12L
#define FREQF_SELECTDIRS	 (1L<<FREQB_SELECTDIRS)

/*****************************************
* flags for RTFO_Flags or fontreq->Flags *
*****************************************/
#define FREQB_FIXEDWIDTH	 5L
#define FREQF_FIXEDWIDTH	 (1L<<FREQB_FIXEDWIDTH)
#define FREQB_COLORFONTS	 6L
#define FREQF_COLORFONTS	 (1L<<FREQB_COLORFONTS)
#define FREQB_CHANGEPALETTE	 7L
#define FREQF_CHANGEPALETTE	 (1L<<FREQB_CHANGEPALETTE)
#define FREQB_LEAVEPALETTE	 8L
#define FREQF_LEAVEPALETTE	 (1L<<FREQB_LEAVEPALETTE)
#define FREQB_SCALE		 9L
#define FREQF_SCALE		 (1L<<FREQB_SCALE)
#define FREQB_STYLE		 10L
#define FREQF_STYLE		 (1L<<FREQB_STYLE)

/*****************************************
* flags for RTEZ_Flags or reqinfo->Flags *
*****************************************/
#define EZREQB_NORETURNKEY	 0L
#define EZREQF_NORETURNKEY	 (1L<<EZREQB_NORETURNKEY)
#define EZREQB_LAMIGAQUAL	 1L
#define EZREQF_LAMIGAQUAL	 (1L<<EZREQB_LAMIGAQUAL)
#define EZREQB_CENTERTEXT	 2L
#define EZREQF_CENTERTEXT	 (1L<<EZREQB_CENTERTEXT)

/********
* hooks *
********/
#define REQHOOK_WILDFILE	 0L
#define REQHOOK_WILDFONT	 1L

#endif /* LIBRARIES_REQTOOLS_H */
