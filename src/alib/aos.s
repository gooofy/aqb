
SysBase          = 4

/* exec */

LVOOpenLibrary   = -552
LVOCloseLibrary  = -414
LVOCopyMem       = -624

/* intuition */

LVOOpenWindow    = -204
LVOCloseWindow   = -72
LVOAllocRemember = -396
LVOFreeRemember  = -408
LVOGetScreenData = -426

    .globl  _OpenLibrary
_OpenLibrary:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1        /* libName      */
    move.l 12(a5),d0        /* version      */
    movea.l SysBase,a6      /* exec base    */
    jsr     LVOOpenLibrary(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _CloseLibrary
_CloseLibrary:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1        /* library      */
    movea.l SysBase,a6      /* exec base    */
    jsr     LVOCloseLibrary(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _CopyMem
_CopyMem:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0        /* source       */
    move.l 12(a5),a1        /* dest         */
    move.l 16(a5),d0        /* size         */
    movea.l SysBase,a6      /* exec base    */
    jsr     LVOCopyMem(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _OpenWindow
_OpenWindow:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0            /* newWindow         */
    movea.l _IntuitionBase,a6   /* intuition base    */
    jsr     LVOOpenWindow(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _CloseWindow
_CloseWindow:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0            /* window            */
    movea.l _IntuitionBase,a6   /* intuition base    */
    jsr     LVOCloseWindow(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _AllocRemember
_AllocRemember:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0            /* rememberKey       */
    move.l 12(a5),d0            /* size              */
    move.l 16(a5),d1            /* flags             */
    movea.l _IntuitionBase,a6   /* intuition base    */
    jsr     LVOAllocRemember(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _FreeRemember
_FreeRemember:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0            /* rememberKey       */
    move.l 12(a5),d0            /* reallyForget      */
    movea.l _IntuitionBase,a6   /* intuition base    */
    jsr     LVOFreeRemember(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _GetScreenData
_GetScreenData:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0            /* buffer            */
    move.l 12(a5),d0            /* size              */
    move.l 16(a5),d1            /* type              */
    move.l 20(a5),a1            /* screen            */
    movea.l _IntuitionBase,a6   /* intuition base    */
    jsr     LVOGetScreenData(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 
 * dos 
 */

LVOWrite  = -48
LVOOutput = -60
LVODelay  = -198

    .globl _Write
_Write:
    link a5, #0
    move.l a6,-(sp)
    move.l d2,-(sp)
    move.l d3,-(sp)

    move.l  8(a5),d1            /* file              */
    move.l 12(a5),d2            /* buffer            */
    move.l 16(a5),d3            /* length            */
    movea.l _DOSBase,a6         /* dos base          */
    jsr     LVOWrite(a6)

    move.l (sp)+,d3
    move.l (sp)+,d2
    move.l (sp)+,a6
    unlk a5
    rts

    .globl _Output
_Output:
    link a5, #0
    move.l a6,-(sp)

    movea.l _DOSBase,a6         /* dos base          */
    jsr     LVOOutput(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _Delay
_Delay:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1            /* ticks             */
    movea.l _DOSBase,a6         /* dos base          */
    jsr     LVODelay(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* graphics */

LVOMove          = -240
LVODraw          = -246
LVORectFill      = -306
LVOSetAPen       = -342
LVOSetBPen       = -348

    .globl _Move
_Move:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1            /* rp                */
    move.l 12(a5),d0            /* x                 */
    move.l 16(a5),d1            /* y                 */
    movea.l _GfxBase,a6         /* gfx base          */
    jsr     LVOMove(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _Draw
_Draw:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1            /* rp                */
    move.l 12(a5),d0            /* x                 */
    move.l 16(a5),d1            /* y                 */
    movea.l _GfxBase,a6         /* gfx base          */
    jsr     LVODraw(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _RectFill
_RectFill:
    link a5, #0
    move.l a6,-(sp)
    move.l d2,-(sp)
    move.l d3,-(sp)

    move.l  8(a5),a1            /* rp                */
    move.l 12(a5),d0            /* xMin              */
    move.l 16(a5),d1            /* yMin              */
    move.l 20(a5),d2            /* xMax              */
    move.l 24(a5),d3            /* yMax              */
    movea.l _GfxBase,a6         /* gfx base          */
    jsr     LVORectFill(a6)

    move.l (sp)+,d3
    move.l (sp)+,d2
    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SetAPen
_SetAPen:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1            /* rp                */
    move.l 12(a5),d0            /* pen               */
    movea.l _GfxBase,a6         /* gfx base          */
    jsr     LVOSetAPen(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SetBPen
_SetBPen:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1            /* rp                */
    move.l 12(a5),d0            /* pen               */
    movea.l _GfxBase,a6         /* gfx base          */
    jsr     LVOSetBPen(a6)

    move.l (sp)+,a6
    unlk a5
    rts

