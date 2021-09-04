#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>
#include <inline/graphics.h>
#include <inline/intuition.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;
extern struct GfxBase       *GfxBase;
extern struct IntuitionBase *IntuitionBase;

static UWORD mypattern[] = { 0xf0f0, 0xf0f0 };

int main (void)
{
	struct Window *win;
	struct MsgPort *port;
	struct IntuiMessage *mess;
	BOOL cont;
	struct RastPort *rp;
	struct TmpRas tmpras;
	UBYTE *tmpbuf;
	ULONG rassize;
	struct AreaInfo areainfo;
	UBYTE *areabuf;

	if (win = OpenWindowTags (NULL,
			                  WA_Left,10,
		                      WA_Top,20,
			                  WA_InnerWidth, 640,
			                  WA_InnerHeight, 200,
			                  WA_Title, (ULONG)"Press Esc to exit",
			                  WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_GIMMEZEROZERO,
			                  WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_VANILLAKEY,
			                  TAG_END))
	{
		rp = win->RPort;

		rassize = RASSIZE(win->GZZWidth,win->GZZHeight);
		if ((tmpbuf = AllocVec (rassize,MEMF_CHIP|MEMF_CLEAR)))
		{
			InitTmpRas (&tmpras,tmpbuf,rassize);
			rp->TmpRas = &tmpras;
		}

		if ((areabuf = AllocVec (5*10, MEMF_CLEAR)))
		{
			InitArea (&areainfo,areabuf, 10);
			rp->AreaInfo = &areainfo;
		}

		SetAPen (rp, 1);
		SetBPen (rp, 2);
		SetDrMd (rp, JAM1);

        rp->AreaPtrn = mypattern;
        rp->AreaPtSz = 1;
		rp->Mask = 3;

		AreaMove (rp,  10,  10);
		AreaDraw (rp, 100,  30);
		AreaDraw (rp,  80, 150);
		AreaEnd(rp);

        rp->AreaPtrn = NULL;
        rp->AreaPtSz = 0;

		AreaMove (rp, 110,  10);
		AreaDraw (rp, 200,  30);
		AreaDraw (rp, 180, 150);
		AreaEnd(rp);

		port = win->UserPort;
		cont = TRUE;
		while (cont)
		{
			WaitPort (port);
			while (mess = (struct IntuiMessage *) GetMsg (port))
			{
				switch (mess->Class)
				{
				case IDCMP_CLOSEWINDOW:
					cont = FALSE;
					break;
				case IDCMP_VANILLAKEY:
					if (mess->Code == 0x1b)
						cont = FALSE;
				}
				ReplyMsg ((struct Message *) mess);
			}
		}

		if (tmpbuf)
		{
			rp->TmpRas = NULL;
			FreeVec (tmpbuf);
		}

		if (areabuf)
		{
			rp->AreaInfo = NULL;
			FreeVec (areabuf);
		}

		CloseWindow (win);
	}
	else
		return (20);

	return (0);
}

