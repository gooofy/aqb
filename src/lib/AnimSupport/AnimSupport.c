#include "../_aqb/_aqb.h"
#include "../_brt/_brt.h"
#include "../IFFSupport/IFFSupport.h"

#include <stdarg.h>

#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <clib/intuition_protos.h>
#include <inline/intuition.h>

#include <clib/graphics_protos.h>
#include <inline/graphics.h>

#include <clib/dos_protos.h>
#include <inline/dos.h>

#include <clib/mathffp_protos.h>
#include <inline/mathffp.h>

#include <proto/console.h>
#include <clib/console_protos.h>
#include <pragmas/console_pragmas.h>

#include "AnimSupport.h"

static BOB_t *g_bob_first               = NULL;
static BOB_t *g_bob_last                = NULL;

static void _cleanupGelSys(struct RastPort *rPort)
{
	struct GelsInfo *gInfo = rPort->GelsInfo;
	if (gInfo)
	{
		DPRINTF ("_cleanupGelSys rPort=0x%08lx, gInfo=0x%08lx\n", rPort, gInfo);
		rPort->GelsInfo = NULL;
		FreeMem(gInfo->collHandler, (LONG)sizeof(struct collTable));
		FreeMem(gInfo->lastColor, (LONG)sizeof(LONG) * 8);
		FreeMem(gInfo->nextLine, (LONG)sizeof(WORD) * 8);
		FreeMem(gInfo->gelHead, (LONG)sizeof(struct VSprite));
		FreeMem(gInfo->gelTail, (LONG)sizeof(struct VSprite));
		FreeMem(gInfo, (LONG)sizeof(*gInfo));
		DPRINTF ("_cleanupGelSys done.\n", rPort, gInfo);
        //Delay(5*50);
	}
}

static void _win_gels_cleanup_cb (struct Window *win)
{
    struct RastPort *rPort = win->RPort;
    DPRINTF ("_win_gels_cleanup_cb called, win=0x%08lx, rPort=0x%08lx\n", win, rPort);
    _cleanupGelSys (rPort);
}

void GELS_INIT (UBYTE sprRsrvd)
{
    DPRINTF ("GELS_INIT sprRsrvd=0x%02x\n", sprRsrvd);
    //Delay(100);

    enum _aqb_output_type ot = _aqb_get_output (/*needGfx=*/TRUE);

    if (_g_cur_rp->GelsInfo)
    {
        ERROR(AE_GELS_INIT);
        return;
    }

	struct GelsInfo *gInfo;
	struct VSprite  *vsHead;
	struct VSprite  *vsTail;

    BOOL ok = FALSE;

	if (NULL != (gInfo = (struct GelsInfo *)AllocMem(sizeof(struct GelsInfo), MEMF_CLEAR)))
	{
		if (NULL != (gInfo->nextLine = (WORD *)AllocMem(sizeof(WORD) * 8, MEMF_CLEAR)))
		{
			if (NULL != (gInfo->lastColor = (WORD **)AllocMem(sizeof(LONG) * 8, MEMF_CLEAR)))
			{
				if (NULL != (gInfo->collHandler = (struct collTable *) AllocMem(sizeof(struct collTable),MEMF_CLEAR)))
				{
					if (NULL != (vsHead = (struct VSprite *) AllocMem((LONG)sizeof(struct VSprite), MEMF_CLEAR)))
					{
						if (NULL != (vsTail = (struct VSprite *) AllocMem(sizeof(struct VSprite), MEMF_CLEAR)))
						{
							gInfo->sprRsrvd   = sprRsrvd;
							/* Set left- and top-most to 1 to better keep items */
							/* inside the display boundaries.                   */
							gInfo->leftmost   = gInfo->topmost    = 1;
							gInfo->rightmost  = (_g_cur_rp->BitMap->BytesPerRow << 3) - 1;
							gInfo->bottommost = _g_cur_rp->BitMap->Rows - 1;
							_g_cur_rp->GelsInfo = gInfo;
							InitGels(vsHead, vsTail, gInfo);
                            ok = TRUE;
                            DPRINTF ("GELS_INIT ok, gInfo=0x%08lx\n", gInfo);

                            switch (ot)
                            {
                                case _aqb_ot_window:
                                    _window_add_close_cb (_win_gels_cleanup_cb);
                                    break;
                                case  _aqb_ot_screen:
                                    // FIXME
                                    break;
                                case _aqb_ot_bitmap:
                                    // FIXME
                                    break;
                                default:
                                    // ???
                                    break;
                            }
                            return;
						}
						FreeMem(vsHead, (LONG)sizeof(*vsHead));
					}
					FreeMem(gInfo->collHandler, (LONG)sizeof(struct collTable));
				}
				FreeMem(gInfo->lastColor, (LONG)sizeof(LONG) * 8);
			}
			FreeMem(gInfo->nextLine, (LONG)sizeof(WORD) * 8);
		}
		FreeMem(gInfo, (LONG)sizeof(*gInfo));
	}

    if (!ok)
        ERROR(AE_GELS_INIT);
}

BOB_t *BOB_ (BITMAP_t *bm)
{
    BOB_t *bob = AllocVec(sizeof(*bob), MEMF_CLEAR);
    if (!bob)
    {
        ERROR(AE_BOB);
        return NULL;
    }

    bob->prev = g_bob_last;
    if (g_bob_last)
        g_bob_last = g_bob_last->next = bob;
    else
        g_bob_first = g_bob_last = bob;

    LONG word_width = (bm->bm.BytesPerRow+1)/2;
    LONG rassize = (LONG)sizeof(UWORD) * word_width * bm->height * bm->bm.Depth;

    bob->bob.SaveBuffer = (WORD *)AllocVec(rassize, MEMF_CHIP);

    if (!bob->bob.SaveBuffer)
    {
        ERROR(AE_BOB);
        return NULL;
    }

    LONG line_size  = 2 * word_width;
    LONG plane_size = line_size * bm->height;

    bob->vsprite.BorderLine = (WORD *)AllocVec(line_size, MEMF_CHIP);
    if (!bob->vsprite.BorderLine)
    {
        ERROR(AE_BOB);
        return NULL;
    }

    bob->vsprite.CollMask = (WORD *)AllocVec(plane_size, MEMF_CHIP);
    if (!bob->vsprite.CollMask)
    {
        ERROR(AE_BOB);
        return NULL;
    }

    bob->vsprite.X          = 0;
    bob->vsprite.Y          = 0;
    bob->vsprite.Flags      = SAVEBACK | OVERLAY;
    bob->vsprite.Width      = word_width;
    bob->vsprite.Depth      = bm->bm.Depth;
    bob->vsprite.Height     = bm->height;
    bob->vsprite.MeMask     = 0;
    bob->vsprite.HitMask    = 0;
    bob->vsprite.ImageData  = (WORD *) bm->bm.Planes[0];
    bob->vsprite.SprColors  = NULL;
    bob->vsprite.PlanePick  = 0xff;
    bob->vsprite.PlaneOnOff = 0x00;

    InitMasks(&bob->vsprite);

	bob->vsprite.VSBob   = &bob->bob;
	bob->bob.BobVSprite  = &bob->vsprite;
	bob->bob.ImageShadow = bob->vsprite.CollMask;
	bob->bob.Flags       = 0;
	bob->bob.Before      = NULL;
	bob->bob.After       = NULL;
	bob->bob.BobComp     = NULL;
	bob->bob.DBuffer     = NULL;
#if 0
	if (nBob->nb_DBuf)
		{
		if (NULL != (bob->DBuffer = (struct DBufPacket *)
				AllocMem((LONG)sizeof(struct DBufPacket), MEMF_CLEAR)))
			{
			if (NULL != (bob->DBuffer->BufBuffer = (WORD *)AllocMem(rassize, MEMF_CHIP)))
				return(bob);
#endif

	DPRINTF ("BOB allocation done, bob=0x%08lx, VSprite: width=%d, height=%d, depth=%d\n", bob, bob->vsprite.Width, bob->vsprite.Height, bob->vsprite.Depth);

    return bob;
}

void BOB_MOVE (BOB_t *bob, BOOL s, SHORT x, SHORT y)
{
    _aqb_get_output (/*needGfx=*/TRUE);

	DPRINTF ("BOB_MOVE bob=0x%08lx, x=%d, y=%d, _g_cur_rp=0x%08lx, _g_cur_vp=0x%08lx\n", bob, x, y, _g_cur_rp, _g_cur_vp);
	if (!bob || !_g_cur_vp || !_g_cur_rp->GelsInfo)
	{
        ERROR(AE_BOB);
        return;
	}

	if (!bob->active)
	{
		DPRINTF ("BOB_MOVE adding bob to rp bob=0x%08lx\n", bob);
        AddBob(&bob->bob, _g_cur_rp);
		bob->active = TRUE;
	}

	bob->bob.BobVSprite->X = x;
	bob->bob.BobVSprite->Y = y;
}

void GELS_REPAINT (void)
{
    _aqb_get_output (/*needGfx=*/TRUE);

	if (!_g_cur_rp->GelsInfo || !_g_cur_vp)
	{
        ERROR(AE_BOB);
        return;
	}

	SortGList(_g_cur_rp);
	DrawGList(_g_cur_rp, _g_cur_vp);
	/* FIXME: If the GelsList includes true VSprites, MrgCop() and LoadView() here */
}

void ILBM_LOAD_BOB (STRPTR path, BOB_t **bob, SHORT scid, ILBM_META_t *pMeta, PALETTE_t *pPalette)
{
    DPRINTF ("ILBM_LOAD_BOB path=%s\n", (char*)path);

    BITMAP_t *bm=NULL;

    ILBM_LOAD_BITMAP (path, &bm, scid, pMeta, pPalette, /*cont=*/TRUE);

    *bob = BOB_(bm);
}

void _AnimSupport_init(void)
{
}

