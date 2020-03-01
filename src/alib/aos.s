
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

/*
 * mathffp
 */

LVOSPFix      = -30
LVOSPFlt      = -36
LVOSPCmp      = -42
LVOSPTst      = -48
LVOSPAbs      = -54
LVOSPNeg      = -60
LVOSPAdd      = -66
LVOSPSub      = -72
LVOSPMul      = -78
LVOSPDiv      = -84
LVOSPFloor    = -90
LVOSPCeil     = -96

    .globl _SPFix
_SPFix:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0
    movea.l _MathBase,a6
    jsr     LVOSPFix(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPFlt
_SPFlt:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0
    movea.l _MathBase,a6
    jsr     LVOSPFlt(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPCmp
_SPCmp:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1
    move.l 12(a5),d0
    movea.l _MathBase,a6
    jsr     LVOSPCmp(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPTst
_SPTst:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1
    movea.l _MathBase,a6
    jsr     LVOSPTst(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPAbs
_SPAbs:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0
    movea.l _MathBase,a6
    jsr     LVOSPAbs(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPNeg
_SPNeg:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0
    movea.l _MathBase,a6
    jsr     LVOSPNeg(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPAdd
_SPAdd:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0
    move.l 12(a5),d1
    movea.l _MathBase,a6
    jsr     LVOSPAdd(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPSub
_SPSub:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0
    move.l 12(a5),d1
    movea.l _MathBase,a6
    jsr     LVOSPSub(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPMul
_SPMul:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0
    move.l 12(a5),d1
    movea.l _MathBase,a6
    jsr     LVOSPMul(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPDiv
_SPDiv:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0
    move.l 12(a5),d1
    movea.l _MathBase,a6
    jsr     LVOSPDiv(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPFloor
_SPFloor:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0
    movea.l _MathBase,a6
    jsr     LVOSPFloor(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPCeil
_SPCeil:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0
    movea.l _MathBase,a6
    jsr     LVOSPCeil(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/*
 * mathtrans
 */


LVOSPAtan   = -30
LVOSPSin    = -36
LVOSPCos    = -42
LVOSPTan    = -48
LVOSPSincos = -54
LVOSPSinh   = -60
LVOSPCosh   = -66
LVOSPTanh   = -72
LVOSPExp    = -78
LVOSPLog    = -84
LVOSPPow    = -90
LVOSPSqrt   = -96
LVOSPTieee  = -102
LVOSPFieee  = -108
LVOSPAsin   = -114
LVOSPAcos   = -120
LVOSPLog10  = -126

    .globl _SPAtan
_SPAtan:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathTransBase,a6
    jsr     LVOSPAtan(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPSin
_SPSin:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathTransBase,a6
    jsr     LVOSPSin(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPCos
_SPCos:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathTransBase,a6
    jsr     LVOSPCos(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPTan
_SPTan:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathTransBase,a6
    jsr     LVOSPTan(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPSincos
_SPSincos:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* cosResult */
    move.l  12(a5),d0 /* parm */
    movea.l _MathTransBase,a6
    jsr     LVOSPSincos(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPSinh
_SPSinh:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathTransBase,a6
    jsr     LVOSPSinh(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPCosh
_SPCosh:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathTransBase,a6
    jsr     LVOSPCosh(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPTanh
_SPTanh:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathTransBase,a6
    jsr     LVOSPTanh(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPExp
_SPExp:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathTransBase,a6
    jsr     LVOSPExp(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPLog
_SPLog:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathTransBase,a6
    jsr     LVOSPLog(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPPow
_SPPow:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* power */
    move.l  12(a5),d0 /* arg */
    movea.l _MathTransBase,a6
    jsr     LVOSPPow(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPSqrt
_SPSqrt:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathTransBase,a6
    jsr     LVOSPSqrt(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPTieee
_SPTieee:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathTransBase,a6
    jsr     LVOSPTieee(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPFieee
_SPFieee:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathTransBase,a6
    jsr     LVOSPFieee(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPAsin
_SPAsin:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathTransBase,a6
    jsr     LVOSPAsin(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPAcos
_SPAcos:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathTransBase,a6
    jsr     LVOSPAcos(a6)

    move.l (sp)+,a6
    unlk a5
    rts

    .globl _SPLog10
_SPLog10:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathTransBase,a6
    jsr     LVOSPLog10(a6)

    move.l (sp)+,a6
    unlk a5
    rts

