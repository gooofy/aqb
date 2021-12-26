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

static SPRITE_t *g_sprite_first = NULL;
static SPRITE_t *g_sprite_last  = NULL;

#define NUM_HW_SPRITES 7

typedef struct
{
    struct ViewPort     *vp;
    struct SimpleSprite  sprite;
} hwsprite_t;

static hwsprite_t  g_hwsprites[NUM_HW_SPRITES];

static BOB_t      *g_bob_first    = NULL;
static BOB_t      *g_bob_last     = NULL;

SPRITE_t *SPRITE_ (BITMAP_t *bm, BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2)
{
    if (!bm)
    {
        DPRINTF ("SPRITE_: !bm\n");
        ERROR(AE_SPRITE);
        return NULL;
    }

    if (x2<0)
        x2 = bm->width-1;
    if (x2 > bm->width-1)
        x2 = bm->width-1;
    if (y2<0)
        y2 = bm->height-1;
    if (y2>bm->height-1)
        y2 = bm->height-1;

    SHORT w = x2-x1+1;
    SHORT h = y2-y1+1;

    if (w>16)
    {
        DPRINTF ("SPRITE_: w=%d > 16\n", w);
        ERROR(AE_SPRITE);
        return NULL;
    }

    UWORD *posctldata = AllocVec(8 + 2*2*h, MEMF_CHIP | MEMF_CLEAR);
    if (!posctldata)
    {
        DPRINTF ("SPRITE: failed to allocate posctldata\n");
        ERROR(AE_SPRITE);
        return NULL;
    }

    for (SHORT y=y1; y<=y2; y++)
    {
        for (SHORT x=x1; x<=x2; x++)
        {
            LONG penno = ReadPixel (&bm->rp, x, y);
            if (penno & 1)
                posctldata[2+(y-y1)*2] |= (1<<(15-(x-x1)));
            if (penno & 2)
                posctldata[2+(y-y1)*2+1] |= (1<<(15-(x-x1)));
        }
    }

    SPRITE_t *sprite = AllocVec(sizeof(*sprite), MEMF_CLEAR);
    if (!sprite)
    {
        DPRINTF ("SPRITE_: failed to allocate sprite mem\n");
        FreeVec (posctldata);
        ERROR(AE_SPRITE);
        return NULL;
    }

    sprite->posctldata = posctldata;
    sprite->width      = w;
    sprite->height     = h;

    sprite->prev = g_sprite_last;
    if (g_sprite_last)
        g_sprite_last = g_sprite_last->next = sprite;
    else
        g_sprite_first = g_sprite_last = sprite;

    return sprite;
}

void SPRITE_SHOW (SHORT spnum, SPRITE_t *sprite)
{
    _aqb_get_output (/*needGfx=*/TRUE);

	DPRINTF ("SPRITE_SHOW sprite=0x%08lx, spnum=%d\n", sprite, spnum);
	if (!sprite || (spnum<1) || (spnum>NUM_HW_SPRITES+1) )
	{
        ERROR(AE_SPRITE);
        return;
	}

    hwsprite_t *hwsp = &g_hwsprites[spnum-1];
    struct ViewPort *vp = hwsp->vp;
    if (!vp)
    {
        vp = _g_cur_vp;

        hwsp->sprite.posctldata = sprite->posctldata;
        hwsp->sprite.height     = sprite->height;
        hwsp->sprite.x          = 0;
        hwsp->sprite.y          = 0;
        hwsp->sprite.num        = 0;

        if (GetSprite (&hwsp->sprite, spnum)<0)
        {
            DPRINTF ("SPRITE_: GetSprite failed\n");
            ERROR(AE_SPRITE);
            return;
        }

        hwsp->vp = vp;
    }
    else
    {
        ChangeSprite (vp, &hwsp->sprite, sprite->posctldata);
    }
}

void SPRITE_HIDE (SHORT spnum)
{
    _aqb_get_output (/*needGfx=*/TRUE);

	DPRINTF ("SPRITE_HIDE spnum=%d\n", spnum);
	if ( (spnum<1) || (spnum>NUM_HW_SPRITES+1) )
	{
        ERROR(AE_SPRITE);
        return;
	}

    hwsprite_t *hwsp = &g_hwsprites[spnum-1];
    if (!hwsp->vp)
	{
        ERROR(AE_SPRITE);
        return;
	}

    FreeSprite (spnum);
    hwsp->vp = NULL;
}

void SPRITE_MOVE (SHORT spnum, BOOL s, SHORT x, SHORT y)
{
	DPRINTF ("SPRITE_MOVE spnum=%d, x=%d, y=%d\n", spnum, x, y);
	if ((spnum<1) || (spnum>NUM_HW_SPRITES+1))
	{
        ERROR(AE_SPRITE);
        return;
	}

    hwsprite_t *hwsp = &g_hwsprites[spnum-1];
    if (!hwsp->vp)
	{
        ERROR(AE_SPRITE);
        return;
	}

    MoveSprite (hwsp->vp, &hwsp->sprite, x, y);
}

void SPRITE_FREE (SPRITE_t *sprite)
{
    DPRINTF ("SPRITE_FREE: sprite=0x%08lx\n", sprite);
	if (!sprite)
	{
        ERROR(AE_SPRITE);
        return;
	}

    if (sprite->prev)
        sprite->prev->next = sprite->next;
    else
        g_sprite_first = sprite->next;

    if (sprite->next)
        sprite->next->prev = sprite->prev;
    else
        g_sprite_last = sprite->prev;

    // if this sprite is still in use as a hw sprite, free that hw sprite

    for (SHORT i=0; i<NUM_HW_SPRITES; i++)
    {
        hwsprite_t *hwsp = &g_hwsprites[i];
        if (!hwsp->vp || (hwsp->sprite.posctldata != sprite->posctldata))
            continue;
        FreeSprite (i+1);
        hwsp->vp = NULL;
    }

    // this sprite may also be in use as a custom mouse pointer

    for (SHORT i=0; i<MAX_NUM_WINDOWS; i++)
    {
        if (!_g_winlist[i] || (_g_winlist[i]->Pointer != sprite->posctldata))
            continue;
        ClearPointer (_g_winlist[i]);
    }

    FreeVec (sprite->posctldata);
    FreeVec (sprite);
}

void ILBM_LOAD_SPRITE (STRPTR path, SPRITE_t **sprite, SHORT scid, ILBM_META_t *pMeta, PALETTE_t *pPalette)
{
    DPRINTF ("ILBM_LOAD_SPRITE path=%s\n", (char*)path);

    BITMAP_t *bm=NULL;

    ILBM_LOAD_BITMAP (path, &bm, scid, pMeta, pPalette, /*cont=*/TRUE);

    *sprite = SPRITE_(bm, FALSE, 0, 0, FALSE, bm->width-1, bm->height-1);
}

void POINTER_SPRITE (SPRITE_t *sprite, SHORT xoffset, SHORT yoffset)
{
    _aqb_get_output (/*needGfx=*/TRUE);

    DPRINTF ("POINTER_SPRITE: sprite=0x%08lx\n", sprite);
	if (!sprite)
	{
        ERROR(AE_SPRITE);
        return;
	}

    SetPointer (_g_cur_win, sprite->posctldata, sprite->height, sprite->width, xoffset, yoffset);
}

void POINTER_CLEAR (void)
{
    _aqb_get_output (/*needGfx=*/TRUE);
    ClearPointer (_g_cur_win);
}

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

    // HIDE all bobs currently active in this rastport:

    for (BOB_t *bob=g_bob_first; bob; bob=bob->next)
    {
        if (bob->rp != rPort)
            continue;
        BOB_HIDE (bob);
    }

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

void BOB_SHOW (BOB_t *bob)
{
    _aqb_get_output (/*needGfx=*/TRUE);

	DPRINTF ("BOB_SHOW bob=0x%08lx, _g_cur_rp=0x%08lx, _g_cur_vp=0x%08lx\n", bob, _g_cur_rp, _g_cur_vp);
	if (!bob || !_g_cur_vp || !_g_cur_rp->GelsInfo)
	{
        ERROR(AE_BOB);
        return;
	}

	if (bob->rp)
        return;

    DPRINTF ("BOB_SHOW adding bob to rp bob=0x%08lx\n", bob);
    AddBob(&bob->bob, _g_cur_rp);
    bob->rp = _g_cur_rp;
}

void BOB_HIDE (BOB_t *bob)
{
    DPRINTF ("BOB_HIDE: bob=0x%08lx\n", bob);
	if (!bob)
	{
        ERROR(AE_BOB);
        return;
	}
    if (!bob->rp)
        return;

    RemBob(&bob->bob);
    bob->rp = NULL;
}

void BOB_FREE (BOB_t *bob)
{
    DPRINTF ("BOB_FREE: bob=0x%08lx\n", bob);
	if (!bob)
	{
        ERROR(AE_BOB);
        return;
	}
    if (bob->rp)
        BOB_HIDE(bob);

    if (bob->prev)
        bob->prev->next = bob->next;
    else
        g_bob_first = bob->next;

    if (bob->next)
        bob->next->prev = bob->prev;
    else
        g_bob_last = bob->prev;

    FreeVec (bob->vsprite.CollMask);
    FreeVec (bob->vsprite.BorderLine);
    FreeVec (bob->bob.SaveBuffer);
    FreeVec (bob);
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

static void _AnimSupport_shutdown(void)
{
    DPRINTF ("_AnimSupport_shutdown called\n");

    BOB_t *bob = g_bob_first;
    while (bob)
    {
        BOB_t *nb = bob->next;
        if (bob->rp)
            BOB_HIDE (bob);
        BOB_FREE (bob);
        bob = nb;
    }

    SPRITE_t *sprite = g_sprite_first;
    while (sprite)
    {
        SPRITE_t *nsp = sprite->next;

        SPRITE_FREE (sprite);
        sprite = nsp;
    }
}

void _AnimSupport_init(void)
{
    for (SHORT i=0; i<NUM_HW_SPRITES; i++)
        g_hwsprites[i].vp = NULL;

    ON_EXIT_CALL(_AnimSupport_shutdown);
}

