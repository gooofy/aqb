
SysBase          = 4

/* intuition */

LVOOpenWindow    = -204
LVOCloseWindow   = -72
LVOAllocRemember = -396
LVOFreeRemember  = -408
LVOGetScreenData = -426

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

/*
 * exec
 */

LVOSupervisor              = -30
LVOExitIntr                = -36
LVOSchedule                = -42
LVOReschedule              = -48
LVOSwitch                  = -54
LVODispatch                = -60
LVOException               = -66
LVOInitCode                = -72
LVOInitStruct              = -78
LVOMakeLibrary             = -84
LVOMakeFunctions           = -90
LVOFindResident            = -96
LVOInitResident            = -102
LVOAlert                   = -108
LVODebug                   = -114
LVODisable                 = -120
LVOEnable                  = -126
LVOForbid                  = -132
LVOPermit                  = -138
LVOSetSR                   = -144
LVOSuperState              = -150
LVOUserState               = -156
LVOSetIntVector            = -162
LVOAddIntServer            = -168
LVORemIntServer            = -174
LVOCause                   = -180
LVOAllocate                = -186
LVODeallocate              = -192
LVOAllocMem                = -198
LVOAllocAbs                = -204
LVOFreeMem                 = -210
LVOAvailMem                = -216
LVOAllocEntry              = -222
LVOFreeEntry               = -228
LVOInsert                  = -234
LVOAddHead                 = -240
LVOAddTail                 = -246
LVORemove                  = -252
LVORemHead                 = -258
LVORemTail                 = -264
LVOEnqueue                 = -270
LVOFindName                = -276
LVOAddTask                 = -282
LVORemTask                 = -288
LVOFindTask                = -294
LVOSetTaskPri              = -300
LVOSetSignal               = -306
LVOSetExcept               = -312
LVOWait                    = -318
LVOSignal                  = -324
LVOAllocSignal             = -330
LVOFreeSignal              = -336
LVOAllocTrap               = -342
LVOFreeTrap                = -348
LVOAddPort                 = -354
LVORemPort                 = -360
LVOPutMsg                  = -366
LVOGetMsg                  = -372
LVOReplyMsg                = -378
LVOWaitPort                = -384
LVOFindPort                = -390
LVOAddLibrary              = -396
LVORemLibrary              = -402
LVOOldOpenLibrary          = -408
LVOCloseLibrary            = -414
LVOSetFunction             = -420
LVOSumLibrary              = -426
LVOAddDevice               = -432
LVORemDevice               = -438
LVOOpenDevice              = -444
LVOCloseDevice             = -450
LVODoIO                    = -456
LVOSendIO                  = -462
LVOCheckIO                 = -468
LVOWaitIO                  = -474
LVOAbortIO                 = -480
LVOAddResource             = -486
LVORemResource             = -492
LVOOpenResource            = -498
LVORawIOInit               = -504
LVORawMayGetChar           = -510
LVORawPutChar              = -516
LVORawDoFmt                = -522
LVOGetCC                   = -528
LVOTypeOfMem               = -534
LVOProcure                 = -540
LVOVacate                  = -546
LVOOpenLibrary             = -552
LVOInitSemaphore           = -558
LVOObtainSemaphore         = -564
LVOReleaseSemaphore        = -570
LVOAttemptSemaphore        = -576
LVOObtainSemaphoreList     = -582
LVOReleaseSemaphoreList    = -588
LVOFindSemaphore           = -594
LVOAddSemaphore            = -600
LVORemSemaphore            = -606
LVOSumKickData             = -612
LVOAddMemList              = -618
LVOCopyMem                 = -624
LVOCopyMemQuick            = -630
LVOCacheClearU             = -636
LVOCacheClearE             = -642
LVOCacheControl            = -648
LVOCreateIORequest         = -654
LVODeleteIORequest         = -660
LVOCreateMsgPort           = -666
LVODeleteMsgPort           = -672
LVOObtainSemaphoreShared   = -678
LVOAllocVec                = -684
LVOFreeVec                 = -690
LVOCreatePrivatePool       = -696
LVODeletePrivatePool       = -702
LVOAllocPooled             = -708
LVOFreePooled              = -714
LVOExecReserved00          = -720
LVOColdReboot              = -726
LVOStackSwap               = -732
LVOChildFree               = -738
LVOChildOrphan             = -744
LVOChildStatus             = -750
LVOChildWait               = -756
LVOExecReserved01          = -762
LVOExecReserved02          = -768
LVOExecReserved03          = -774
LVOExecReserved04          = -780
/* 30 $ffe2 -$001e Supervisor(userFunction)(a5) */
    .globl _Supervisor
_Supervisor:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a5 /* userFunction */
    movea.l SysBase,a6
    jsr     LVOSupervisor(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 36 $ffdc -$0024 ExitIntr()() */
    .globl _ExitIntr
_ExitIntr:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVOExitIntr(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 42 $ffd6 -$002a Schedule()() */
    .globl _Schedule
_Schedule:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVOSchedule(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 48 $ffd0 -$0030 Reschedule()() */
    .globl _Reschedule
_Reschedule:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVOReschedule(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 54 $ffca -$0036 Switch()() */
    .globl _Switch
_Switch:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVOSwitch(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 60 $ffc4 -$003c Dispatch()() */
    .globl _Dispatch
_Dispatch:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVODispatch(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 66 $ffbe -$0042 Exception()() */
    .globl _Exception
_Exception:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVOException(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 72 $ffb8 -$0048 InitCode(startClass,version)(d0/d1) */
    .globl _InitCode
_InitCode:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* startClass */
    move.l  12(a5),d1 /* version */
    movea.l SysBase,a6
    jsr     LVOInitCode(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 78 $ffb2 -$004e InitStruct(initTable,memory,size)(a1/a2,d0) */
    .globl _InitStruct
_InitStruct:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* initTable */
    move.l  12(a5),a2 /* memory */
    move.l  16(a5),d0 /* size */
    movea.l SysBase,a6
    jsr     LVOInitStruct(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 84 $ffac -$0054 MakeLibrary(funcInit,structInit,libInit,dataSize,segList)(a0/a1/a2,d0/d1) */
    .globl _MakeLibrary
_MakeLibrary:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* funcInit */
    move.l  12(a5),a1 /* structInit */
    move.l  16(a5),a2 /* libInit */
    move.l  20(a5),d0 /* dataSize */
    move.l  24(a5),d1 /* segList */
    movea.l SysBase,a6
    jsr     LVOMakeLibrary(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 90 $ffa6 -$005a MakeFunctions(target,functionArray,funcDispBase)(a0/a1/a2) */
    .globl _MakeFunctions
_MakeFunctions:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* target */
    move.l  12(a5),a1 /* functionArray */
    move.l  16(a5),a2 /* funcDispBase */
    movea.l SysBase,a6
    jsr     LVOMakeFunctions(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 96 $ffa0 -$0060 FindResident(name)(a1) */
    .globl _FindResident
_FindResident:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* name */
    movea.l SysBase,a6
    jsr     LVOFindResident(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 102 $ff9a -$0066 InitResident(resident,segList)(a1,d1) */
    .globl _InitResident
_InitResident:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* resident */
    move.l  12(a5),d1 /* segList */
    movea.l SysBase,a6
    jsr     LVOInitResident(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 108 $ff94 -$006c Alert(alertNum)(d7) */
    .globl _Alert
_Alert:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d7 /* alertNum */
    movea.l SysBase,a6
    jsr     LVOAlert(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 114 $ff8e -$0072 Debug(flags)(d0) */
    .globl _Debug
_Debug:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* flags */
    movea.l SysBase,a6
    jsr     LVODebug(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 120 $ff88 -$0078 Disable()() */
    .globl _Disable
_Disable:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVODisable(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 126 $ff82 -$007e Enable()() */
    .globl _Enable
_Enable:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVOEnable(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 132 $ff7c -$0084 Forbid()() */
    .globl _Forbid
_Forbid:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVOForbid(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 138 $ff76 -$008a Permit()() */
    .globl _Permit
_Permit:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVOPermit(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 144 $ff70 -$0090 SetSR(newSR,mask)(d0/d1) */
    .globl _SetSR
_SetSR:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* newSR */
    move.l  12(a5),d1 /* mask */
    movea.l SysBase,a6
    jsr     LVOSetSR(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 150 $ff6a -$0096 SuperState()() */
    .globl _SuperState
_SuperState:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVOSuperState(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 156 $ff64 -$009c UserState(sysStack)(d0) */
    .globl _UserState
_UserState:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* sysStack */
    movea.l SysBase,a6
    jsr     LVOUserState(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 162 $ff5e -$00a2 SetIntVector(intNumber,interrupt)(d0/a1) */
    .globl _SetIntVector
_SetIntVector:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* intNumber */
    move.l  12(a5),a1 /* interrupt */
    movea.l SysBase,a6
    jsr     LVOSetIntVector(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 168 $ff58 -$00a8 AddIntServer(intNumber,interrupt)(d0/a1) */
    .globl _AddIntServer
_AddIntServer:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* intNumber */
    move.l  12(a5),a1 /* interrupt */
    movea.l SysBase,a6
    jsr     LVOAddIntServer(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 174 $ff52 -$00ae RemIntServer(intNumber,interrupt)(d0/a1) */
    .globl _RemIntServer
_RemIntServer:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* intNumber */
    move.l  12(a5),a1 /* interrupt */
    movea.l SysBase,a6
    jsr     LVORemIntServer(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 180 $ff4c -$00b4 Cause(interrupt)(a1) */
    .globl _Cause
_Cause:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* interrupt */
    movea.l SysBase,a6
    jsr     LVOCause(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 186 $ff46 -$00ba Allocate(freeList,byteSize)(a0,d0) */
    .globl _Allocate
_Allocate:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* freeList */
    move.l  12(a5),d0 /* byteSize */
    movea.l SysBase,a6
    jsr     LVOAllocate(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 192 $ff40 -$00c0 Deallocate(freeList,memoryBlock,byteSize)(a0/a1,d0) */
    .globl _Deallocate
_Deallocate:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* freeList */
    move.l  12(a5),a1 /* memoryBlock */
    move.l  16(a5),d0 /* byteSize */
    movea.l SysBase,a6
    jsr     LVODeallocate(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 198 $ff3a -$00c6 AllocMem(byteSize,requirements)(d0/d1) */
    .globl _AllocMem
_AllocMem:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* byteSize */
    move.l  12(a5),d1 /* requirements */
    movea.l SysBase,a6
    jsr     LVOAllocMem(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 204 $ff34 -$00cc AllocAbs(byteSize,location)(d0/a1) */
    .globl _AllocAbs
_AllocAbs:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* byteSize */
    move.l  12(a5),a1 /* location */
    movea.l SysBase,a6
    jsr     LVOAllocAbs(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 210 $ff2e -$00d2 FreeMem(memoryBlock,byteSize)(a1,d0) */
    .globl _FreeMem
_FreeMem:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* memoryBlock */
    move.l  12(a5),d0 /* byteSize */
    movea.l SysBase,a6
    jsr     LVOFreeMem(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 216 $ff28 -$00d8 AvailMem(requirements)(d1) */
    .globl _AvailMem
_AvailMem:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* requirements */
    movea.l SysBase,a6
    jsr     LVOAvailMem(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 222 $ff22 -$00de AllocEntry(entry)(a0) */
    .globl _AllocEntry
_AllocEntry:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* entry */
    movea.l SysBase,a6
    jsr     LVOAllocEntry(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 228 $ff1c -$00e4 FreeEntry(entry)(a0) */
    .globl _FreeEntry
_FreeEntry:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* entry */
    movea.l SysBase,a6
    jsr     LVOFreeEntry(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 234 $ff16 -$00ea Insert(list,node,pred)(a0/a1/a2) */
    .globl _Insert
_Insert:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* list */
    move.l  12(a5),a1 /* node */
    move.l  16(a5),a2 /* pred */
    movea.l SysBase,a6
    jsr     LVOInsert(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 240 $ff10 -$00f0 AddHead(list,node)(a0/a1) */
    .globl _AddHead
_AddHead:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* list */
    move.l  12(a5),a1 /* node */
    movea.l SysBase,a6
    jsr     LVOAddHead(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 246 $ff0a -$00f6 AddTail(list,node)(a0/a1) */
    .globl _AddTail
_AddTail:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* list */
    move.l  12(a5),a1 /* node */
    movea.l SysBase,a6
    jsr     LVOAddTail(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 252 $ff04 -$00fc Remove(node)(a1) */
    .globl _Remove
_Remove:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* node */
    movea.l SysBase,a6
    jsr     LVORemove(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 258 $fefe -$0102 RemHead(list)(a0) */
    .globl _RemHead
_RemHead:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* list */
    movea.l SysBase,a6
    jsr     LVORemHead(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 264 $fef8 -$0108 RemTail(list)(a0) */
    .globl _RemTail
_RemTail:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* list */
    movea.l SysBase,a6
    jsr     LVORemTail(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 270 $fef2 -$010e Enqueue(list,node)(a0/a1) */
    .globl _Enqueue
_Enqueue:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* list */
    move.l  12(a5),a1 /* node */
    movea.l SysBase,a6
    jsr     LVOEnqueue(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 276 $feec -$0114 FindName(list,name)(a0/a1) */
    .globl _FindName
_FindName:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* list */
    move.l  12(a5),a1 /* name */
    movea.l SysBase,a6
    jsr     LVOFindName(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 282 $fee6 -$011a AddTask(task,initPC,finalPC)(a1/a2/a3) */
    .globl _AddTask
_AddTask:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* task */
    move.l  12(a5),a2 /* initPC */
    move.l  16(a5),a3 /* finalPC */
    movea.l SysBase,a6
    jsr     LVOAddTask(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 288 $fee0 -$0120 RemTask(task)(a1) */
    .globl _RemTask
_RemTask:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* task */
    movea.l SysBase,a6
    jsr     LVORemTask(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 294 $feda -$0126 FindTask(name)(a1) */
    .globl _FindTask
_FindTask:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* name */
    movea.l SysBase,a6
    jsr     LVOFindTask(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 300 $fed4 -$012c SetTaskPri(task,priority)(a1,d0) */
    .globl _SetTaskPri
_SetTaskPri:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* task */
    move.l  12(a5),d0 /* priority */
    movea.l SysBase,a6
    jsr     LVOSetTaskPri(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 306 $fece -$0132 SetSignal(newSignals,signalSet)(d0/d1) */
    .globl _SetSignal
_SetSignal:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* newSignals */
    move.l  12(a5),d1 /* signalSet */
    movea.l SysBase,a6
    jsr     LVOSetSignal(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 312 $fec8 -$0138 SetExcept(newSignals,signalSet)(d0/d1) */
    .globl _SetExcept
_SetExcept:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* newSignals */
    move.l  12(a5),d1 /* signalSet */
    movea.l SysBase,a6
    jsr     LVOSetExcept(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 318 $fec2 -$013e Wait(signalSet)(d0) */
    .globl _Wait
_Wait:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* signalSet */
    movea.l SysBase,a6
    jsr     LVOWait(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 324 $febc -$0144 Signal(task,signalSet)(a1,d0) */
    .globl _Signal
_Signal:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* task */
    move.l  12(a5),d0 /* signalSet */
    movea.l SysBase,a6
    jsr     LVOSignal(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 330 $feb6 -$014a AllocSignal(signalNum)(d0) */
    .globl _AllocSignal
_AllocSignal:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* signalNum */
    movea.l SysBase,a6
    jsr     LVOAllocSignal(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 336 $feb0 -$0150 FreeSignal(signalNum)(d0) */
    .globl _FreeSignal
_FreeSignal:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* signalNum */
    movea.l SysBase,a6
    jsr     LVOFreeSignal(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 342 $feaa -$0156 AllocTrap(trapNum)(d0) */
    .globl _AllocTrap
_AllocTrap:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* trapNum */
    movea.l SysBase,a6
    jsr     LVOAllocTrap(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 348 $fea4 -$015c FreeTrap(trapNum)(d0) */
    .globl _FreeTrap
_FreeTrap:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* trapNum */
    movea.l SysBase,a6
    jsr     LVOFreeTrap(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 354 $fe9e -$0162 AddPort(port)(a1) */
    .globl _AddPort
_AddPort:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* port */
    movea.l SysBase,a6
    jsr     LVOAddPort(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 360 $fe98 -$0168 RemPort(port)(a1) */
    .globl _RemPort
_RemPort:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* port */
    movea.l SysBase,a6
    jsr     LVORemPort(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 366 $fe92 -$016e PutMsg(port,message)(a0/a1) */
    .globl _PutMsg
_PutMsg:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* port */
    move.l  12(a5),a1 /* message */
    movea.l SysBase,a6
    jsr     LVOPutMsg(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 372 $fe8c -$0174 GetMsg(port)(a0) */
    .globl _GetMsg
_GetMsg:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* port */
    movea.l SysBase,a6
    jsr     LVOGetMsg(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 378 $fe86 -$017a ReplyMsg(message)(a1) */
    .globl _ReplyMsg
_ReplyMsg:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* message */
    movea.l SysBase,a6
    jsr     LVOReplyMsg(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 384 $fe80 -$0180 WaitPort(port)(a0) */
    .globl _WaitPort
_WaitPort:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* port */
    movea.l SysBase,a6
    jsr     LVOWaitPort(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 390 $fe7a -$0186 FindPort(name)(a1) */
    .globl _FindPort
_FindPort:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* name */
    movea.l SysBase,a6
    jsr     LVOFindPort(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 396 $fe74 -$018c AddLibrary(library)(a1) */
    .globl _AddLibrary
_AddLibrary:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* library */
    movea.l SysBase,a6
    jsr     LVOAddLibrary(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 402 $fe6e -$0192 RemLibrary(library)(a1) */
    .globl _RemLibrary
_RemLibrary:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* library */
    movea.l SysBase,a6
    jsr     LVORemLibrary(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 408 $fe68 -$0198 OldOpenLibrary(libName)(a1) */
    .globl _OldOpenLibrary
_OldOpenLibrary:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* libName */
    movea.l SysBase,a6
    jsr     LVOOldOpenLibrary(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 414 $fe62 -$019e CloseLibrary(library)(a1) */
    .globl _CloseLibrary
_CloseLibrary:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* library */
    movea.l SysBase,a6
    jsr     LVOCloseLibrary(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 420 $fe5c -$01a4 SetFunction(library,funcOffset,newFunction)(a1,a0,d0) */
    .globl _SetFunction
_SetFunction:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* library */
    move.l  12(a5),a0 /* funcOffset */
    move.l  16(a5),d0 /* newFunction */
    movea.l SysBase,a6
    jsr     LVOSetFunction(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 426 $fe56 -$01aa SumLibrary(library)(a1) */
    .globl _SumLibrary
_SumLibrary:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* library */
    movea.l SysBase,a6
    jsr     LVOSumLibrary(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 432 $fe50 -$01b0 AddDevice(device)(a1) */
    .globl _AddDevice
_AddDevice:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* device */
    movea.l SysBase,a6
    jsr     LVOAddDevice(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 438 $fe4a -$01b6 RemDevice(device)(a1) */
    .globl _RemDevice
_RemDevice:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* device */
    movea.l SysBase,a6
    jsr     LVORemDevice(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 444 $fe44 -$01bc OpenDevice(devName,unit,ioRequest,flags)(a0,d0/a1,d1) */
    .globl _OpenDevice
_OpenDevice:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* devName */
    move.l  12(a5),d0 /* unit */
    move.l  16(a5),a1 /* ioRequest */
    move.l  20(a5),d1 /* flags */
    movea.l SysBase,a6
    jsr     LVOOpenDevice(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 450 $fe3e -$01c2 CloseDevice(ioRequest)(a1) */
    .globl _CloseDevice
_CloseDevice:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* ioRequest */
    movea.l SysBase,a6
    jsr     LVOCloseDevice(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 456 $fe38 -$01c8 DoIO(ioRequest)(a1) */
    .globl _DoIO
_DoIO:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* ioRequest */
    movea.l SysBase,a6
    jsr     LVODoIO(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 462 $fe32 -$01ce SendIO(ioRequest)(a1) */
    .globl _SendIO
_SendIO:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* ioRequest */
    movea.l SysBase,a6
    jsr     LVOSendIO(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 468 $fe2c -$01d4 CheckIO(ioRequest)(a1) */
    .globl _CheckIO
_CheckIO:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* ioRequest */
    movea.l SysBase,a6
    jsr     LVOCheckIO(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 474 $fe26 -$01da WaitIO(ioRequest)(a1) */
    .globl _WaitIO
_WaitIO:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* ioRequest */
    movea.l SysBase,a6
    jsr     LVOWaitIO(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 480 $fe20 -$01e0 AbortIO(ioRequest)(a1) */
    .globl _AbortIO
_AbortIO:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* ioRequest */
    movea.l SysBase,a6
    jsr     LVOAbortIO(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 486 $fe1a -$01e6 AddResource(resource)(a1) */
    .globl _AddResource
_AddResource:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* resource */
    movea.l SysBase,a6
    jsr     LVOAddResource(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 492 $fe14 -$01ec RemResource(resource)(a1) */
    .globl _RemResource
_RemResource:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* resource */
    movea.l SysBase,a6
    jsr     LVORemResource(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 498 $fe0e -$01f2 OpenResource(resName)(a1) */
    .globl _OpenResource
_OpenResource:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* resName */
    movea.l SysBase,a6
    jsr     LVOOpenResource(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 504 $fe08 -$01f8 RawIOInit()() */
    .globl _RawIOInit
_RawIOInit:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVORawIOInit(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 510 $fe02 -$01fe RawMayGetChar()() */
    .globl _RawMayGetChar
_RawMayGetChar:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVORawMayGetChar(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 516 $fdfc -$0204 RawPutChar()() */
    .globl _RawPutChar
_RawPutChar:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVORawPutChar(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 522 $fdf6 -$020a RawDoFmt(formatString,dataStream,putChProc,putChData)(a0/a1/a2/a3) */
    .globl _RawDoFmt
_RawDoFmt:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* formatString */
    move.l  12(a5),a1 /* dataStream */
    move.l  16(a5),a2 /* putChProc */
    move.l  20(a5),a3 /* putChData */
    movea.l SysBase,a6
    jsr     LVORawDoFmt(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 528 $fdf0 -$0210 GetCC()() */
    .globl _GetCC
_GetCC:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVOGetCC(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 534 $fdea -$0216 TypeOfMem(address)(a1) */
    .globl _TypeOfMem
_TypeOfMem:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* address */
    movea.l SysBase,a6
    jsr     LVOTypeOfMem(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 540 $fde4 -$021c Procure(semaport,bidMsg)(a0/a1) */
    .globl _Procure
_Procure:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* semaport */
    move.l  12(a5),a1 /* bidMsg */
    movea.l SysBase,a6
    jsr     LVOProcure(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 546 $fdde -$0222 Vacate(semaport)(a0) */
    .globl _Vacate
_Vacate:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* semaport */
    movea.l SysBase,a6
    jsr     LVOVacate(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 552 $fdd8 -$0228 OpenLibrary(libName,version)(a1,d0) */
    .globl _OpenLibrary
_OpenLibrary:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* libName */
    move.l  12(a5),d0 /* version */
    movea.l SysBase,a6
    jsr     LVOOpenLibrary(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 558 $fdd2 -$022e InitSemaphore(sigSem)(a0) */
    .globl _InitSemaphore
_InitSemaphore:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* sigSem */
    movea.l SysBase,a6
    jsr     LVOInitSemaphore(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 564 $fdcc -$0234 ObtainSemaphore(sigSem)(a0) */
    .globl _ObtainSemaphore
_ObtainSemaphore:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* sigSem */
    movea.l SysBase,a6
    jsr     LVOObtainSemaphore(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 570 $fdc6 -$023a ReleaseSemaphore(sigSem)(a0) */
    .globl _ReleaseSemaphore
_ReleaseSemaphore:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* sigSem */
    movea.l SysBase,a6
    jsr     LVOReleaseSemaphore(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 576 $fdc0 -$0240 AttemptSemaphore(sigSem)(a0) */
    .globl _AttemptSemaphore
_AttemptSemaphore:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* sigSem */
    movea.l SysBase,a6
    jsr     LVOAttemptSemaphore(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 582 $fdba -$0246 ObtainSemaphoreList(sigSem)(a0) */
    .globl _ObtainSemaphoreList
_ObtainSemaphoreList:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* sigSem */
    movea.l SysBase,a6
    jsr     LVOObtainSemaphoreList(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 588 $fdb4 -$024c ReleaseSemaphoreList(sigSem)(a0) */
    .globl _ReleaseSemaphoreList
_ReleaseSemaphoreList:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* sigSem */
    movea.l SysBase,a6
    jsr     LVOReleaseSemaphoreList(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 594 $fdae -$0252 FindSemaphore(sigSem)(a1) */
    .globl _FindSemaphore
_FindSemaphore:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* sigSem */
    movea.l SysBase,a6
    jsr     LVOFindSemaphore(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 600 $fda8 -$0258 AddSemaphore(sigSem)(a1) */
    .globl _AddSemaphore
_AddSemaphore:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* sigSem */
    movea.l SysBase,a6
    jsr     LVOAddSemaphore(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 606 $fda2 -$025e RemSemaphore(sigSem)(a1) */
    .globl _RemSemaphore
_RemSemaphore:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* sigSem */
    movea.l SysBase,a6
    jsr     LVORemSemaphore(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 612 $fd9c -$0264 SumKickData()() */
    .globl _SumKickData
_SumKickData:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVOSumKickData(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 618 $fd96 -$026a AddMemList(size,attributes,pri,base,name)(d0/d1/d2/a0/a1) */
    .globl _AddMemList
_AddMemList:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* size */
    move.l  12(a5),d1 /* attributes */
    move.l  16(a5),d2 /* pri */
    move.l  20(a5),a0 /* base */
    move.l  24(a5),a1 /* name */
    movea.l SysBase,a6
    jsr     LVOAddMemList(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 624 $fd90 -$0270 CopyMem(source,dest,size)(a0/a1,d0) */
    .globl _CopyMem
_CopyMem:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* source */
    move.l  12(a5),a1 /* dest */
    move.l  16(a5),d0 /* size */
    movea.l SysBase,a6
    jsr     LVOCopyMem(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 630 $fd8a -$0276 CopyMemQuick(source,dest,size)(a0/a1,d0) */
    .globl _CopyMemQuick
_CopyMemQuick:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* source */
    move.l  12(a5),a1 /* dest */
    move.l  16(a5),d0 /* size */
    movea.l SysBase,a6
    jsr     LVOCopyMemQuick(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 636 $fd84 -$027c CacheClearU()() */
    .globl _CacheClearU
_CacheClearU:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVOCacheClearU(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 642 $fd7e -$0282 CacheClearE(address,length,caches)(a0,d0/d1) */
    .globl _CacheClearE
_CacheClearE:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* address */
    move.l  12(a5),d0 /* length */
    move.l  16(a5),d1 /* caches */
    movea.l SysBase,a6
    jsr     LVOCacheClearE(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 648 $fd78 -$0288 CacheControl(cacheBits,cacheMask)(d0/d1) */
    .globl _CacheControl
_CacheControl:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* cacheBits */
    move.l  12(a5),d1 /* cacheMask */
    movea.l SysBase,a6
    jsr     LVOCacheControl(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 654 $fd72 -$028e CreateIORequest(port,size)(a0,d0) */
    .globl _CreateIORequest
_CreateIORequest:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* port */
    move.l  12(a5),d0 /* size */
    movea.l SysBase,a6
    jsr     LVOCreateIORequest(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 660 $fd6c -$0294 DeleteIORequest(iorequest)(a0) */
    .globl _DeleteIORequest
_DeleteIORequest:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* iorequest */
    movea.l SysBase,a6
    jsr     LVODeleteIORequest(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 666 $fd66 -$029a CreateMsgPort()() */
    .globl _CreateMsgPort
_CreateMsgPort:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVOCreateMsgPort(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 672 $fd60 -$02a0 DeleteMsgPort(port)(a0) */
    .globl _DeleteMsgPort
_DeleteMsgPort:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* port */
    movea.l SysBase,a6
    jsr     LVODeleteMsgPort(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 678 $fd5a -$02a6 ObtainSemaphoreShared(sigSem)(a0) */
    .globl _ObtainSemaphoreShared
_ObtainSemaphoreShared:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* sigSem */
    movea.l SysBase,a6
    jsr     LVOObtainSemaphoreShared(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 684 $fd54 -$02ac AllocVec(byteSize,requirements)(d0/d1) */
    .globl _AllocVec
_AllocVec:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* byteSize */
    move.l  12(a5),d1 /* requirements */
    movea.l SysBase,a6
    jsr     LVOAllocVec(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 690 $fd4e -$02b2 FreeVec(memoryBlock)(a1) */
    .globl _FreeVec
_FreeVec:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* memoryBlock */
    movea.l SysBase,a6
    jsr     LVOFreeVec(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 696 $fd48 -$02b8 CreatePrivatePool(requirements,puddleSize,puddleThresh)(d0/d1/d2) */
    .globl _CreatePrivatePool
_CreatePrivatePool:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* requirements */
    move.l  12(a5),d1 /* puddleSize */
    move.l  16(a5),d2 /* puddleThresh */
    movea.l SysBase,a6
    jsr     LVOCreatePrivatePool(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 702 $fd42 -$02be DeletePrivatePool(poolHeader)(a0) */
    .globl _DeletePrivatePool
_DeletePrivatePool:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* poolHeader */
    movea.l SysBase,a6
    jsr     LVODeletePrivatePool(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 708 $fd3c -$02c4 AllocPooled(memSize,poolHeader)(d0/a0) */
    .globl _AllocPooled
_AllocPooled:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* memSize */
    move.l  12(a5),a0 /* poolHeader */
    movea.l SysBase,a6
    jsr     LVOAllocPooled(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 714 $fd36 -$02ca FreePooled(memory,poolHeader)(a1,a0) */
    .globl _FreePooled
_FreePooled:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* memory */
    move.l  12(a5),a0 /* poolHeader */
    movea.l SysBase,a6
    jsr     LVOFreePooled(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 720 $fd30 -$02d0 ExecReserved00(nothing)(d0) */
    .globl _ExecReserved00
_ExecReserved00:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* nothing */
    movea.l SysBase,a6
    jsr     LVOExecReserved00(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 726 $fd2a -$02d6 ColdReboot()() */
    .globl _ColdReboot
_ColdReboot:
    link a5, #0
    move.l a6,-(sp)

    movea.l SysBase,a6
    jsr     LVOColdReboot(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 732 $fd24 -$02dc StackSwap(newSize,newSP,newStack)(d0/d1/a0) */
    .globl _StackSwap
_StackSwap:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* newSize */
    move.l  12(a5),d1 /* newSP */
    move.l  16(a5),a0 /* newStack */
    movea.l SysBase,a6
    jsr     LVOStackSwap(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 738 $fd1e -$02e2 ChildFree(tid)(d0) */
    .globl _ChildFree
_ChildFree:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* tid */
    movea.l SysBase,a6
    jsr     LVOChildFree(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 744 $fd18 -$02e8 ChildOrphan(tid)(d0) */
    .globl _ChildOrphan
_ChildOrphan:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* tid */
    movea.l SysBase,a6
    jsr     LVOChildOrphan(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 750 $fd12 -$02ee ChildStatus(tid)(d0) */
    .globl _ChildStatus
_ChildStatus:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* tid */
    movea.l SysBase,a6
    jsr     LVOChildStatus(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 756 $fd0c -$02f4 ChildWait(tid)(d0) */
    .globl _ChildWait
_ChildWait:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* tid */
    movea.l SysBase,a6
    jsr     LVOChildWait(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 762 $fd06 -$02fa ExecReserved01(nothing)(d0) */
    .globl _ExecReserved01
_ExecReserved01:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* nothing */
    movea.l SysBase,a6
    jsr     LVOExecReserved01(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 768 $fd00 -$0300 ExecReserved02(nothing)(d0) */
    .globl _ExecReserved02
_ExecReserved02:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* nothing */
    movea.l SysBase,a6
    jsr     LVOExecReserved02(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 774 $fcfa -$0306 ExecReserved03(nothing)(d0) */
    .globl _ExecReserved03
_ExecReserved03:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* nothing */
    movea.l SysBase,a6
    jsr     LVOExecReserved03(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 780 $fcf4 -$030c ExecReserved04(nothing)(d0) */
    .globl _ExecReserved04
_ExecReserved04:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* nothing */
    movea.l SysBase,a6
    jsr     LVOExecReserved04(a6)

    move.l (sp)+,a6
    unlk a5
    rts

