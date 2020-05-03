
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

/*
 * graphics
 */

LVOBltBitMap               = -30
LVOBltTemplate             = -36
LVOClearEOL                = -42
LVOClearScreen             = -48
LVOTextLength              = -54
LVOText                    = -60
LVOSetFont                 = -66
LVOOpenFont                = -72
LVOCloseFont               = -78
LVOAskSoftStyle            = -84
LVOSetSoftStyle            = -90
LVOAddBob                  = -96
LVOAddVSprite              = -102
LVODoCollision             = -108
LVODrawGList               = -114
LVOInitGels                = -120
LVOInitMasks               = -126
LVORemIBob                 = -132
LVORemVSprite              = -138
LVOSetCollision            = -144
LVOSortGList               = -150
LVOAddAnimOb               = -156
LVOAnimate                 = -162
LVOGetGBuffers             = -168
LVOInitGMasks              = -174
LVODrawEllipse             = -180
LVOAreaEllipse             = -186
LVOLoadRGB4                = -192
LVOInitRastPort            = -198
LVOInitVPort               = -204
LVOMrgCop                  = -210
LVOMakeVPort               = -216
LVOLoadView                = -222
LVOWaitBlit                = -228
LVOSetRast                 = -234
LVOMove                    = -240
LVODraw                    = -246
LVOAreaMove                = -252
LVOAreaDraw                = -258
LVOAreaEnd                 = -264
LVOWaitTOF                 = -270
LVOQBlit                   = -276
LVOInitArea                = -282
LVOSetRGB4                 = -288
LVOQBSBlit                 = -294
LVOBltClear                = -300
LVORectFill                = -306
LVOBltPattern              = -312
LVOReadPixel               = -318
LVOWritePixel              = -324
LVOFlood                   = -330
LVOPolyDraw                = -336
LVOSetAPen                 = -342
LVOSetBPen                 = -348
LVOSetDrMd                 = -354
LVOInitView                = -360
LVOCBump                   = -366
LVOCMove                   = -372
LVOCWait                   = -378
LVOVBeamPos                = -384
LVOInitBitMap              = -390
LVOScrollRaster            = -396
LVOWaitBOVP                = -402
LVOGetSprite               = -408
LVOFreeSprite              = -414
LVOChangeSprite            = -420
LVOMoveSprite              = -426
LVOLockLayerRom            = -432
LVOUnlockLayerRom          = -438
LVOSyncSBitMap             = -444
LVOCopySBitMap             = -450
LVOOwnBlitter              = -456
LVODisownBlitter           = -462
LVOInitTmpRas              = -468
LVOAskFont                 = -474
LVOAddFont                 = -480
LVORemFont                 = -486
LVOAllocRaster             = -492
LVOFreeRaster              = -498
LVOAndRectRegion           = -504
LVOOrRectRegion            = -510
LVONewRegion               = -516
LVOClearRectRegion         = -522
LVOClearRegion             = -528
LVODisposeRegion           = -534
LVOFreeVPortCopLists       = -540
LVOFreeCopList             = -546
LVOClipBlit                = -552
LVOXorRectRegion           = -558
LVOFreeCprList             = -564
LVOGetColorMap             = -570
LVOFreeColorMap            = -576
LVOGetRGB4                 = -582
LVOScrollVPort             = -588
LVOUCopperListInit         = -594
LVOFreeGBuffers            = -600
LVOBltBitMapRastPort       = -606
LVOOrRegionRegion          = -612
LVOXorRegionRegion         = -618
LVOAndRegionRegion         = -624
LVOSetRGB4CM               = -630
LVOBltMaskBitMapRastPort   = -636
LVOAttemptLockLayerRom     = -654
/* 30 $ffe2 -$001e BltBitMap(srcBitMap,xSrc,ySrc,destBitMap,xDest,yDest,xSize,ySize,minterm,mask,tempA)(a0,d0/d1/a1,d2/d3/d4/d5/d6/d7/a2) */
    .globl _BltBitMap
_BltBitMap:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* srcBitMap */
    move.l  12(a5),d0 /* xSrc */
    move.l  16(a5),d1 /* ySrc */
    move.l  20(a5),a1 /* destBitMap */
    move.l  24(a5),d2 /* xDest */
    move.l  28(a5),d3 /* yDest */
    move.l  32(a5),d4 /* xSize */
    move.l  36(a5),d5 /* ySize */
    move.l  40(a5),d6 /* minterm */
    move.l  44(a5),d7 /* mask */
    move.l  48(a5),a2 /* tempA */
    movea.l _GfxBase,a6
    jsr     LVOBltBitMap(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 36 $ffdc -$0024 BltTemplate(source,xSrc,srcMod,destRP,xDest,yDest,xSize,ySize)(a0,d0/d1/a1,d2/d3/d4/d5) */
    .globl _BltTemplate
_BltTemplate:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* source */
    move.l  12(a5),d0 /* xSrc */
    move.l  16(a5),d1 /* srcMod */
    move.l  20(a5),a1 /* destRP */
    move.l  24(a5),d2 /* xDest */
    move.l  28(a5),d3 /* yDest */
    move.l  32(a5),d4 /* xSize */
    move.l  36(a5),d5 /* ySize */
    movea.l _GfxBase,a6
    jsr     LVOBltTemplate(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 42 $ffd6 -$002a ClearEOL(rp)(a1) */
    .globl _ClearEOL
_ClearEOL:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    movea.l _GfxBase,a6
    jsr     LVOClearEOL(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 48 $ffd0 -$0030 ClearScreen(rp)(a1) */
    .globl _ClearScreen
_ClearScreen:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    movea.l _GfxBase,a6
    jsr     LVOClearScreen(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 54 $ffca -$0036 TextLength(rp,string,count)(a1,a0,d0) */
    .globl _TextLength
_TextLength:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),a0 /* string */
    move.l  16(a5),d0 /* count */
    movea.l _GfxBase,a6
    jsr     LVOTextLength(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 60 $ffc4 -$003c Text(rp,string,count)(a1,a0,d0) */
    .globl _Text
_Text:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),a0 /* string */
    move.l  16(a5),d0 /* count */
    movea.l _GfxBase,a6
    jsr     LVOText(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 66 $ffbe -$0042 SetFont(rp,textFont)(a1,a0) */
    .globl _SetFont
_SetFont:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),a0 /* textFont */
    movea.l _GfxBase,a6
    jsr     LVOSetFont(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 72 $ffb8 -$0048 OpenFont(textAttr)(a0) */
    .globl _OpenFont
_OpenFont:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* textAttr */
    movea.l _GfxBase,a6
    jsr     LVOOpenFont(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 78 $ffb2 -$004e CloseFont(textFont)(a1) */
    .globl _CloseFont
_CloseFont:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* textFont */
    movea.l _GfxBase,a6
    jsr     LVOCloseFont(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 84 $ffac -$0054 AskSoftStyle(rp)(a1) */
    .globl _AskSoftStyle
_AskSoftStyle:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    movea.l _GfxBase,a6
    jsr     LVOAskSoftStyle(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 90 $ffa6 -$005a SetSoftStyle(rp,style,enable)(a1,d0/d1) */
    .globl _SetSoftStyle
_SetSoftStyle:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d0 /* style */
    move.l  16(a5),d1 /* enable */
    movea.l _GfxBase,a6
    jsr     LVOSetSoftStyle(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 96 $ffa0 -$0060 AddBob(bob,rp)(a0/a1) */
    .globl _AddBob
_AddBob:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* bob */
    move.l  12(a5),a1 /* rp */
    movea.l _GfxBase,a6
    jsr     LVOAddBob(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 102 $ff9a -$0066 AddVSprite(vSprite,rp)(a0/a1) */
    .globl _AddVSprite
_AddVSprite:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* vSprite */
    move.l  12(a5),a1 /* rp */
    movea.l _GfxBase,a6
    jsr     LVOAddVSprite(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 108 $ff94 -$006c DoCollision(rp)(a1) */
    .globl _DoCollision
_DoCollision:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    movea.l _GfxBase,a6
    jsr     LVODoCollision(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 114 $ff8e -$0072 DrawGList(rp,vp)(a1,a0) */
    .globl _DrawGList
_DrawGList:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),a0 /* vp */
    movea.l _GfxBase,a6
    jsr     LVODrawGList(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 120 $ff88 -$0078 InitGels(head,tail,gelsInfo)(a0/a1/a2) */
    .globl _InitGels
_InitGels:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* head */
    move.l  12(a5),a1 /* tail */
    move.l  16(a5),a2 /* gelsInfo */
    movea.l _GfxBase,a6
    jsr     LVOInitGels(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 126 $ff82 -$007e InitMasks(vSprite)(a0) */
    .globl _InitMasks
_InitMasks:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* vSprite */
    movea.l _GfxBase,a6
    jsr     LVOInitMasks(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 132 $ff7c -$0084 RemIBob(bob,rp,vp)(a0/a1/a2) */
    .globl _RemIBob
_RemIBob:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* bob */
    move.l  12(a5),a1 /* rp */
    move.l  16(a5),a2 /* vp */
    movea.l _GfxBase,a6
    jsr     LVORemIBob(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 138 $ff76 -$008a RemVSprite(vSprite)(a0) */
    .globl _RemVSprite
_RemVSprite:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* vSprite */
    movea.l _GfxBase,a6
    jsr     LVORemVSprite(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 144 $ff70 -$0090 SetCollision(num,routine,gelsInfo)(d0/a0/a1) */
    .globl _SetCollision
_SetCollision:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* num */
    move.l  12(a5),a0 /* routine */
    move.l  16(a5),a1 /* gelsInfo */
    movea.l _GfxBase,a6
    jsr     LVOSetCollision(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 150 $ff6a -$0096 SortGList(rp)(a1) */
    .globl _SortGList
_SortGList:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    movea.l _GfxBase,a6
    jsr     LVOSortGList(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 156 $ff64 -$009c AddAnimOb(anOb,anKey,rp)(a0/a1/a2) */
    .globl _AddAnimOb
_AddAnimOb:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* anOb */
    move.l  12(a5),a1 /* anKey */
    move.l  16(a5),a2 /* rp */
    movea.l _GfxBase,a6
    jsr     LVOAddAnimOb(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 162 $ff5e -$00a2 Animate(anKey,rp)(a0/a1) */
    .globl _Animate
_Animate:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* anKey */
    move.l  12(a5),a1 /* rp */
    movea.l _GfxBase,a6
    jsr     LVOAnimate(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 168 $ff58 -$00a8 GetGBuffers(anOb,rp,flag)(a0/a1,d0) */
    .globl _GetGBuffers
_GetGBuffers:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* anOb */
    move.l  12(a5),a1 /* rp */
    move.l  16(a5),d0 /* flag */
    movea.l _GfxBase,a6
    jsr     LVOGetGBuffers(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 174 $ff52 -$00ae InitGMasks(anOb)(a0) */
    .globl _InitGMasks
_InitGMasks:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* anOb */
    movea.l _GfxBase,a6
    jsr     LVOInitGMasks(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 180 $ff4c -$00b4 DrawEllipse(rp,xCenter,yCenter,a,b)(a1,d0/d1/d2/d3) */
    .globl _DrawEllipse
_DrawEllipse:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d0 /* xCenter */
    move.l  16(a5),d1 /* yCenter */
    move.l  20(a5),d2 /* a */
    move.l  24(a5),d3 /* b */
    movea.l _GfxBase,a6
    jsr     LVODrawEllipse(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 186 $ff46 -$00ba AreaEllipse(rp,xCenter,yCenter,a,b)(a1,d0/d1/d2/d3) */
    .globl _AreaEllipse
_AreaEllipse:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d0 /* xCenter */
    move.l  16(a5),d1 /* yCenter */
    move.l  20(a5),d2 /* a */
    move.l  24(a5),d3 /* b */
    movea.l _GfxBase,a6
    jsr     LVOAreaEllipse(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 192 $ff40 -$00c0 LoadRGB4(vp,colors,count)(a0/a1,d0) */
    .globl _LoadRGB4
_LoadRGB4:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* vp */
    move.l  12(a5),a1 /* colors */
    move.l  16(a5),d0 /* count */
    movea.l _GfxBase,a6
    jsr     LVOLoadRGB4(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 198 $ff3a -$00c6 InitRastPort(rp)(a1) */
    .globl _InitRastPort
_InitRastPort:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    movea.l _GfxBase,a6
    jsr     LVOInitRastPort(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 204 $ff34 -$00cc InitVPort(vp)(a0) */
    .globl _InitVPort
_InitVPort:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* vp */
    movea.l _GfxBase,a6
    jsr     LVOInitVPort(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 210 $ff2e -$00d2 MrgCop(view)(a1) */
    .globl _MrgCop
_MrgCop:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* view */
    movea.l _GfxBase,a6
    jsr     LVOMrgCop(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 216 $ff28 -$00d8 MakeVPort(view,vp)(a0/a1) */
    .globl _MakeVPort
_MakeVPort:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* view */
    move.l  12(a5),a1 /* vp */
    movea.l _GfxBase,a6
    jsr     LVOMakeVPort(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 222 $ff22 -$00de LoadView(view)(a1) */
    .globl _LoadView
_LoadView:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* view */
    movea.l _GfxBase,a6
    jsr     LVOLoadView(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 228 $ff1c -$00e4 WaitBlit()() */
    .globl _WaitBlit
_WaitBlit:
    link a5, #0
    move.l a6,-(sp)

    movea.l _GfxBase,a6
    jsr     LVOWaitBlit(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 234 $ff16 -$00ea SetRast(rp,pen)(a1,d0) */
    .globl _SetRast
_SetRast:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d0 /* pen */
    movea.l _GfxBase,a6
    jsr     LVOSetRast(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 240 $ff10 -$00f0 Move(rp,x,y)(a1,d0/d1) */
    .globl _Move
_Move:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d0 /* x */
    move.l  16(a5),d1 /* y */
    movea.l _GfxBase,a6
    jsr     LVOMove(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 246 $ff0a -$00f6 Draw(rp,x,y)(a1,d0/d1) */
    .globl _Draw
_Draw:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d0 /* x */
    move.l  16(a5),d1 /* y */
    movea.l _GfxBase,a6
    jsr     LVODraw(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 252 $ff04 -$00fc AreaMove(rp,x,y)(a1,d0/d1) */
    .globl _AreaMove
_AreaMove:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d0 /* x */
    move.l  16(a5),d1 /* y */
    movea.l _GfxBase,a6
    jsr     LVOAreaMove(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 258 $fefe -$0102 AreaDraw(rp,x,y)(a1,d0/d1) */
    .globl _AreaDraw
_AreaDraw:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d0 /* x */
    move.l  16(a5),d1 /* y */
    movea.l _GfxBase,a6
    jsr     LVOAreaDraw(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 264 $fef8 -$0108 AreaEnd(rp)(a1) */
    .globl _AreaEnd
_AreaEnd:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    movea.l _GfxBase,a6
    jsr     LVOAreaEnd(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 270 $fef2 -$010e WaitTOF()() */
    .globl _WaitTOF
_WaitTOF:
    link a5, #0
    move.l a6,-(sp)

    movea.l _GfxBase,a6
    jsr     LVOWaitTOF(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 276 $feec -$0114 QBlit(blit)(a1) */
    .globl _QBlit
_QBlit:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* blit */
    movea.l _GfxBase,a6
    jsr     LVOQBlit(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 282 $fee6 -$011a InitArea(areaInfo,vectorBuffer,maxVectors)(a0/a1,d0) */
    .globl _InitArea
_InitArea:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* areaInfo */
    move.l  12(a5),a1 /* vectorBuffer */
    move.l  16(a5),d0 /* maxVectors */
    movea.l _GfxBase,a6
    jsr     LVOInitArea(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 288 $fee0 -$0120 SetRGB4(vp,index,red,green,blue)(a0,d0/d1/d2/d3) */
    .globl _SetRGB4
_SetRGB4:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* vp */
    move.l  12(a5),d0 /* index */
    move.l  16(a5),d1 /* red */
    move.l  20(a5),d2 /* green */
    move.l  24(a5),d3 /* blue */
    movea.l _GfxBase,a6
    jsr     LVOSetRGB4(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 294 $feda -$0126 QBSBlit(blit)(a1) */
    .globl _QBSBlit
_QBSBlit:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* blit */
    movea.l _GfxBase,a6
    jsr     LVOQBSBlit(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 300 $fed4 -$012c BltClear(memBlock,byteCount,flags)(a1,d0/d1) */
    .globl _BltClear
_BltClear:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* memBlock */
    move.l  12(a5),d0 /* byteCount */
    move.l  16(a5),d1 /* flags */
    movea.l _GfxBase,a6
    jsr     LVOBltClear(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 306 $fece -$0132 RectFill(rp,xMin,yMin,xMax,yMax)(a1,d0/d1/d2/d3) */
    .globl _RectFill
_RectFill:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d0 /* xMin */
    move.l  16(a5),d1 /* yMin */
    move.l  20(a5),d2 /* xMax */
    move.l  24(a5),d3 /* yMax */
    movea.l _GfxBase,a6
    jsr     LVORectFill(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 312 $fec8 -$0138 BltPattern(rp,mask,xMin,yMin,xMax,yMax,maskBPR)(a1,a0,d0/d1/d2/d3/d4) */
    .globl _BltPattern
_BltPattern:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),a0 /* mask */
    move.l  16(a5),d0 /* xMin */
    move.l  20(a5),d1 /* yMin */
    move.l  24(a5),d2 /* xMax */
    move.l  28(a5),d3 /* yMax */
    move.l  32(a5),d4 /* maskBPR */
    movea.l _GfxBase,a6
    jsr     LVOBltPattern(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 318 $fec2 -$013e ReadPixel(rp,x,y)(a1,d0/d1) */
    .globl _ReadPixel
_ReadPixel:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d0 /* x */
    move.l  16(a5),d1 /* y */
    movea.l _GfxBase,a6
    jsr     LVOReadPixel(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 324 $febc -$0144 WritePixel(rp,x,y)(a1,d0/d1) */
    .globl _WritePixel
_WritePixel:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d0 /* x */
    move.l  16(a5),d1 /* y */
    movea.l _GfxBase,a6
    jsr     LVOWritePixel(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 330 $feb6 -$014a Flood(rp,mode,x,y)(a1,d2,d0/d1) */
    .globl _Flood
_Flood:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d2 /* mode */
    move.l  16(a5),d0 /* x */
    move.l  20(a5),d1 /* y */
    movea.l _GfxBase,a6
    jsr     LVOFlood(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 336 $feb0 -$0150 PolyDraw(rp,count,polyTable)(a1,d0/a0) */
    .globl _PolyDraw
_PolyDraw:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d0 /* count */
    move.l  16(a5),a0 /* polyTable */
    movea.l _GfxBase,a6
    jsr     LVOPolyDraw(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 342 $feaa -$0156 SetAPen(rp,pen)(a1,d0) */
    .globl _SetAPen
_SetAPen:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d0 /* pen */
    movea.l _GfxBase,a6
    jsr     LVOSetAPen(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 348 $fea4 -$015c SetBPen(rp,pen)(a1,d0) */
    .globl _SetBPen
_SetBPen:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d0 /* pen */
    movea.l _GfxBase,a6
    jsr     LVOSetBPen(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 354 $fe9e -$0162 SetDrMd(rp,drawMode)(a1,d0) */
    .globl _SetDrMd
_SetDrMd:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d0 /* drawMode */
    movea.l _GfxBase,a6
    jsr     LVOSetDrMd(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 360 $fe98 -$0168 InitView(view)(a1) */
    .globl _InitView
_InitView:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* view */
    movea.l _GfxBase,a6
    jsr     LVOInitView(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 366 $fe92 -$016e CBump(copList)(a1) */
    .globl _CBump
_CBump:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* copList */
    movea.l _GfxBase,a6
    jsr     LVOCBump(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 372 $fe8c -$0174 CMove(copList,destination,data)(a1,d0/d1) */
    .globl _CMove
_CMove:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* copList */
    move.l  12(a5),d0 /* destination */
    move.l  16(a5),d1 /* data */
    movea.l _GfxBase,a6
    jsr     LVOCMove(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 378 $fe86 -$017a CWait(copList,v,h)(a1,d0/d1) */
    .globl _CWait
_CWait:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* copList */
    move.l  12(a5),d0 /* v */
    move.l  16(a5),d1 /* h */
    movea.l _GfxBase,a6
    jsr     LVOCWait(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 384 $fe80 -$0180 VBeamPos()() */
    .globl _VBeamPos
_VBeamPos:
    link a5, #0
    move.l a6,-(sp)

    movea.l _GfxBase,a6
    jsr     LVOVBeamPos(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 390 $fe7a -$0186 InitBitMap(bitMap,depth,width,height)(a0,d0/d1/d2) */
    .globl _InitBitMap
_InitBitMap:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* bitMap */
    move.l  12(a5),d0 /* depth */
    move.l  16(a5),d1 /* width */
    move.l  20(a5),d2 /* height */
    movea.l _GfxBase,a6
    jsr     LVOInitBitMap(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 396 $fe74 -$018c ScrollRaster(rp,dx,dy,xMin,yMin,xMax,yMax)(a1,d0/d1/d2/d3/d4/d5) */
    .globl _ScrollRaster
_ScrollRaster:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),d0 /* dx */
    move.l  16(a5),d1 /* dy */
    move.l  20(a5),d2 /* xMin */
    move.l  24(a5),d3 /* yMin */
    move.l  28(a5),d4 /* xMax */
    move.l  32(a5),d5 /* yMax */
    movea.l _GfxBase,a6
    jsr     LVOScrollRaster(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 402 $fe6e -$0192 WaitBOVP(vp)(a0) */
    .globl _WaitBOVP
_WaitBOVP:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* vp */
    movea.l _GfxBase,a6
    jsr     LVOWaitBOVP(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 408 $fe68 -$0198 GetSprite(sprite,num)(a0,d0) */
    .globl _GetSprite
_GetSprite:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* sprite */
    move.l  12(a5),d0 /* num */
    movea.l _GfxBase,a6
    jsr     LVOGetSprite(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 414 $fe62 -$019e FreeSprite(num)(d0) */
    .globl _FreeSprite
_FreeSprite:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* num */
    movea.l _GfxBase,a6
    jsr     LVOFreeSprite(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 420 $fe5c -$01a4 ChangeSprite(vp,sprite,newData)(a0/a1/a2) */
    .globl _ChangeSprite
_ChangeSprite:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* vp */
    move.l  12(a5),a1 /* sprite */
    move.l  16(a5),a2 /* newData */
    movea.l _GfxBase,a6
    jsr     LVOChangeSprite(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 426 $fe56 -$01aa MoveSprite(vp,sprite,x,y)(a0/a1,d0/d1) */
    .globl _MoveSprite
_MoveSprite:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* vp */
    move.l  12(a5),a1 /* sprite */
    move.l  16(a5),d0 /* x */
    move.l  20(a5),d1 /* y */
    movea.l _GfxBase,a6
    jsr     LVOMoveSprite(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 432 $fe50 -$01b0 LockLayerRom(layer)(a5) */
    .globl _LockLayerRom
_LockLayerRom:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a5 /* layer */
    movea.l _GfxBase,a6
    jsr     LVOLockLayerRom(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 438 $fe4a -$01b6 UnlockLayerRom(layer)(a5) */
    .globl _UnlockLayerRom
_UnlockLayerRom:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a5 /* layer */
    movea.l _GfxBase,a6
    jsr     LVOUnlockLayerRom(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 444 $fe44 -$01bc SyncSBitMap(layer)(a0) */
    .globl _SyncSBitMap
_SyncSBitMap:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* layer */
    movea.l _GfxBase,a6
    jsr     LVOSyncSBitMap(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 450 $fe3e -$01c2 CopySBitMap(layer)(a0) */
    .globl _CopySBitMap
_CopySBitMap:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* layer */
    movea.l _GfxBase,a6
    jsr     LVOCopySBitMap(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 456 $fe38 -$01c8 OwnBlitter()() */
    .globl _OwnBlitter
_OwnBlitter:
    link a5, #0
    move.l a6,-(sp)

    movea.l _GfxBase,a6
    jsr     LVOOwnBlitter(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 462 $fe32 -$01ce DisownBlitter()() */
    .globl _DisownBlitter
_DisownBlitter:
    link a5, #0
    move.l a6,-(sp)

    movea.l _GfxBase,a6
    jsr     LVODisownBlitter(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 468 $fe2c -$01d4 InitTmpRas(tmpRas,buffer,size)(a0/a1,d0) */
    .globl _InitTmpRas
_InitTmpRas:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* tmpRas */
    move.l  12(a5),a1 /* buffer */
    move.l  16(a5),d0 /* size */
    movea.l _GfxBase,a6
    jsr     LVOInitTmpRas(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 474 $fe26 -$01da AskFont(rp,textAttr)(a1,a0) */
    .globl _AskFont
_AskFont:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* rp */
    move.l  12(a5),a0 /* textAttr */
    movea.l _GfxBase,a6
    jsr     LVOAskFont(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 480 $fe20 -$01e0 AddFont(textFont)(a1) */
    .globl _AddFont
_AddFont:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* textFont */
    movea.l _GfxBase,a6
    jsr     LVOAddFont(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 486 $fe1a -$01e6 RemFont(textFont)(a1) */
    .globl _RemFont
_RemFont:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* textFont */
    movea.l _GfxBase,a6
    jsr     LVORemFont(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 492 $fe14 -$01ec AllocRaster(width,height)(d0/d1) */
    .globl _AllocRaster
_AllocRaster:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* width */
    move.l  12(a5),d1 /* height */
    movea.l _GfxBase,a6
    jsr     LVOAllocRaster(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 498 $fe0e -$01f2 FreeRaster(p,width,height)(a0,d0/d1) */
    .globl _FreeRaster
_FreeRaster:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* p */
    move.l  12(a5),d0 /* width */
    move.l  16(a5),d1 /* height */
    movea.l _GfxBase,a6
    jsr     LVOFreeRaster(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 504 $fe08 -$01f8 AndRectRegion(region,rectangle)(a0/a1) */
    .globl _AndRectRegion
_AndRectRegion:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* region */
    move.l  12(a5),a1 /* rectangle */
    movea.l _GfxBase,a6
    jsr     LVOAndRectRegion(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 510 $fe02 -$01fe OrRectRegion(region,rectangle)(a0/a1) */
    .globl _OrRectRegion
_OrRectRegion:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* region */
    move.l  12(a5),a1 /* rectangle */
    movea.l _GfxBase,a6
    jsr     LVOOrRectRegion(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 516 $fdfc -$0204 NewRegion()() */
    .globl _NewRegion
_NewRegion:
    link a5, #0
    move.l a6,-(sp)

    movea.l _GfxBase,a6
    jsr     LVONewRegion(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 522 $fdf6 -$020a ClearRectRegion(region,rectangle)(a0/a1) */
    .globl _ClearRectRegion
_ClearRectRegion:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* region */
    move.l  12(a5),a1 /* rectangle */
    movea.l _GfxBase,a6
    jsr     LVOClearRectRegion(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 528 $fdf0 -$0210 ClearRegion(region)(a0) */
    .globl _ClearRegion
_ClearRegion:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* region */
    movea.l _GfxBase,a6
    jsr     LVOClearRegion(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 534 $fdea -$0216 DisposeRegion(region)(a0) */
    .globl _DisposeRegion
_DisposeRegion:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* region */
    movea.l _GfxBase,a6
    jsr     LVODisposeRegion(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 540 $fde4 -$021c FreeVPortCopLists(vp)(a0) */
    .globl _FreeVPortCopLists
_FreeVPortCopLists:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* vp */
    movea.l _GfxBase,a6
    jsr     LVOFreeVPortCopLists(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 546 $fdde -$0222 FreeCopList(copList)(a0) */
    .globl _FreeCopList
_FreeCopList:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* copList */
    movea.l _GfxBase,a6
    jsr     LVOFreeCopList(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 552 $fdd8 -$0228 ClipBlit(srcRP,xSrc,ySrc,destRP,xDest,yDest,xSize,ySize,minterm)(a0,d0/d1/a1,d2/d3/d4/d5/d6) */
    .globl _ClipBlit
_ClipBlit:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* srcRP */
    move.l  12(a5),d0 /* xSrc */
    move.l  16(a5),d1 /* ySrc */
    move.l  20(a5),a1 /* destRP */
    move.l  24(a5),d2 /* xDest */
    move.l  28(a5),d3 /* yDest */
    move.l  32(a5),d4 /* xSize */
    move.l  36(a5),d5 /* ySize */
    move.l  40(a5),d6 /* minterm */
    movea.l _GfxBase,a6
    jsr     LVOClipBlit(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 558 $fdd2 -$022e XorRectRegion(region,rectangle)(a0/a1) */
    .globl _XorRectRegion
_XorRectRegion:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* region */
    move.l  12(a5),a1 /* rectangle */
    movea.l _GfxBase,a6
    jsr     LVOXorRectRegion(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 564 $fdcc -$0234 FreeCprList(cprList)(a0) */
    .globl _FreeCprList
_FreeCprList:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* cprList */
    movea.l _GfxBase,a6
    jsr     LVOFreeCprList(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 570 $fdc6 -$023a GetColorMap(entries)(d0) */
    .globl _GetColorMap
_GetColorMap:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* entries */
    movea.l _GfxBase,a6
    jsr     LVOGetColorMap(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 576 $fdc0 -$0240 FreeColorMap(colorMap)(a0) */
    .globl _FreeColorMap
_FreeColorMap:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* colorMap */
    movea.l _GfxBase,a6
    jsr     LVOFreeColorMap(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 582 $fdba -$0246 GetRGB4(colorMap,entry)(a0,d0) */
    .globl _GetRGB4
_GetRGB4:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* colorMap */
    move.l  12(a5),d0 /* entry */
    movea.l _GfxBase,a6
    jsr     LVOGetRGB4(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 588 $fdb4 -$024c ScrollVPort(vp)(a0) */
    .globl _ScrollVPort
_ScrollVPort:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* vp */
    movea.l _GfxBase,a6
    jsr     LVOScrollVPort(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 594 $fdae -$0252 UCopperListInit(uCopList,n)(a0,d0) */
    .globl _UCopperListInit
_UCopperListInit:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* uCopList */
    move.l  12(a5),d0 /* n */
    movea.l _GfxBase,a6
    jsr     LVOUCopperListInit(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 600 $fda8 -$0258 FreeGBuffers(anOb,rp,flag)(a0/a1,d0) */
    .globl _FreeGBuffers
_FreeGBuffers:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* anOb */
    move.l  12(a5),a1 /* rp */
    move.l  16(a5),d0 /* flag */
    movea.l _GfxBase,a6
    jsr     LVOFreeGBuffers(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 606 $fda2 -$025e BltBitMapRastPort(srcBitMap,xSrc,ySrc,destRP,xDest,yDest,xSize,ySize,minterm)(a0,d0/d1/a1,d2/d3/d4/d5/d6) */
    .globl _BltBitMapRastPort
_BltBitMapRastPort:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* srcBitMap */
    move.l  12(a5),d0 /* xSrc */
    move.l  16(a5),d1 /* ySrc */
    move.l  20(a5),a1 /* destRP */
    move.l  24(a5),d2 /* xDest */
    move.l  28(a5),d3 /* yDest */
    move.l  32(a5),d4 /* xSize */
    move.l  36(a5),d5 /* ySize */
    move.l  40(a5),d6 /* minterm */
    movea.l _GfxBase,a6
    jsr     LVOBltBitMapRastPort(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 612 $fd9c -$0264 OrRegionRegion(srcRegion,destRegion)(a0/a1) */
    .globl _OrRegionRegion
_OrRegionRegion:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* srcRegion */
    move.l  12(a5),a1 /* destRegion */
    movea.l _GfxBase,a6
    jsr     LVOOrRegionRegion(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 618 $fd96 -$026a XorRegionRegion(srcRegion,destRegion)(a0/a1) */
    .globl _XorRegionRegion
_XorRegionRegion:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* srcRegion */
    move.l  12(a5),a1 /* destRegion */
    movea.l _GfxBase,a6
    jsr     LVOXorRegionRegion(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 624 $fd90 -$0270 AndRegionRegion(srcRegion,destRegion)(a0/a1) */
    .globl _AndRegionRegion
_AndRegionRegion:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* srcRegion */
    move.l  12(a5),a1 /* destRegion */
    movea.l _GfxBase,a6
    jsr     LVOAndRegionRegion(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 630 $fd8a -$0276 SetRGB4CM(colorMap,index,red,green,blue)(a0,d0/d1/d2/d3) */
    .globl _SetRGB4CM
_SetRGB4CM:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* colorMap */
    move.l  12(a5),d0 /* index */
    move.l  16(a5),d1 /* red */
    move.l  20(a5),d2 /* green */
    move.l  24(a5),d3 /* blue */
    movea.l _GfxBase,a6
    jsr     LVOSetRGB4CM(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 636 $fd84 -$027c BltMaskBitMapRastPort(srcBitMap,xSrc,ySrc,destRP,xDest,yDest,xSize,ySize,minterm,bltMask)(a0,d0/d1/a1,d2/d3/d4/d5/d6/a2) */
    .globl _BltMaskBitMapRastPort
_BltMaskBitMapRastPort:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a0 /* srcBitMap */
    move.l  12(a5),d0 /* xSrc */
    move.l  16(a5),d1 /* ySrc */
    move.l  20(a5),a1 /* destRP */
    move.l  24(a5),d2 /* xDest */
    move.l  28(a5),d3 /* yDest */
    move.l  32(a5),d4 /* xSize */
    move.l  36(a5),d5 /* ySize */
    move.l  40(a5),d6 /* minterm */
    move.l  44(a5),a2 /* bltMask */
    movea.l _GfxBase,a6
    jsr     LVOBltMaskBitMapRastPort(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 654 $fd72 -$028e AttemptLockLayerRom(layer)(a5) */
    .globl _AttemptLockLayerRom
_AttemptLockLayerRom:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a5 /* layer */
    movea.l _GfxBase,a6
    jsr     LVOAttemptLockLayerRom(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/*
 * dos
 */

LVOOpen                    = -30
LVOClose                   = -36
LVORead                    = -42
LVOWrite                   = -48
LVOInput                   = -54
LVOOutput                  = -60
LVOSeek                    = -66
LVODeleteFile              = -72
LVORename                  = -78
LVOLock                    = -84
LVOUnLock                  = -90
LVODupLock                 = -96
LVOExamine                 = -102
LVOExNext                  = -108
LVOInfo                    = -114
LVOCreateDir               = -120
LVOCurrentDir              = -126
LVOIoErr                   = -132
LVOCreateProc              = -138
LVOExit                    = -144
LVOLoadSeg                 = -150
LVOUnLoadSeg               = -156
LVODeviceProc              = -174
LVOSetComment              = -180
LVOSetProtection           = -186
LVODateStamp               = -192
LVODelay                   = -198
LVOWaitForChar             = -204
LVOParentDir               = -210
LVOIsInteractive           = -216
LVOExecute                 = -222
/* 30 $ffe2 -$001e Open(name,accessMode)(d1/d2) */
    .globl _Open
_Open:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* name */
    move.l  12(a5),d2 /* accessMode */
    movea.l _DOSBase,a6
    jsr     LVOOpen(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 36 $ffdc -$0024 Close(file)(d1) */
    .globl _Close
_Close:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* file */
    movea.l _DOSBase,a6
    jsr     LVOClose(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 42 $ffd6 -$002a Read(file,buffer,length)(d1/d2/d3) */
    .globl _Read
_Read:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* file */
    move.l  12(a5),d2 /* buffer */
    move.l  16(a5),d3 /* length */
    movea.l _DOSBase,a6
    jsr     LVORead(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 48 $ffd0 -$0030 Write(file,buffer,length)(d1/d2/d3) */
    .globl _Write
_Write:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* file */
    move.l  12(a5),d2 /* buffer */
    move.l  16(a5),d3 /* length */
    movea.l _DOSBase,a6
    jsr     LVOWrite(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 54 $ffca -$0036 Input()() */
    .globl _Input
_Input:
    link a5, #0
    move.l a6,-(sp)

    movea.l _DOSBase,a6
    jsr     LVOInput(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 60 $ffc4 -$003c Output()() */
    .globl _Output
_Output:
    link a5, #0
    move.l a6,-(sp)

    movea.l _DOSBase,a6
    jsr     LVOOutput(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 66 $ffbe -$0042 Seek(file,position,offset)(d1/d2/d3) */
    .globl _Seek
_Seek:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* file */
    move.l  12(a5),d2 /* position */
    move.l  16(a5),d3 /* offset */
    movea.l _DOSBase,a6
    jsr     LVOSeek(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 72 $ffb8 -$0048 DeleteFile(name)(d1) */
    .globl _DeleteFile
_DeleteFile:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* name */
    movea.l _DOSBase,a6
    jsr     LVODeleteFile(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 78 $ffb2 -$004e Rename(oldName,newName)(d1/d2) */
    .globl _Rename
_Rename:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* oldName */
    move.l  12(a5),d2 /* newName */
    movea.l _DOSBase,a6
    jsr     LVORename(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 84 $ffac -$0054 Lock(name,type)(d1/d2) */
    .globl _Lock
_Lock:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* name */
    move.l  12(a5),d2 /* type */
    movea.l _DOSBase,a6
    jsr     LVOLock(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 90 $ffa6 -$005a UnLock(lock)(d1) */
    .globl _UnLock
_UnLock:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* lock */
    movea.l _DOSBase,a6
    jsr     LVOUnLock(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 96 $ffa0 -$0060 DupLock(lock)(d1) */
    .globl _DupLock
_DupLock:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* lock */
    movea.l _DOSBase,a6
    jsr     LVODupLock(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 102 $ff9a -$0066 Examine(lock,fileInfoBlock)(d1/d2) */
    .globl _Examine
_Examine:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* lock */
    move.l  12(a5),d2 /* fileInfoBlock */
    movea.l _DOSBase,a6
    jsr     LVOExamine(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 108 $ff94 -$006c ExNext(lock,fileInfoBlock)(d1/d2) */
    .globl _ExNext
_ExNext:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* lock */
    move.l  12(a5),d2 /* fileInfoBlock */
    movea.l _DOSBase,a6
    jsr     LVOExNext(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 114 $ff8e -$0072 Info(lock,parameterBlock)(d1/d2) */
    .globl _Info
_Info:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* lock */
    move.l  12(a5),d2 /* parameterBlock */
    movea.l _DOSBase,a6
    jsr     LVOInfo(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 120 $ff88 -$0078 CreateDir(name)(d1) */
    .globl _CreateDir
_CreateDir:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* name */
    movea.l _DOSBase,a6
    jsr     LVOCreateDir(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 126 $ff82 -$007e CurrentDir(lock)(d1) */
    .globl _CurrentDir
_CurrentDir:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* lock */
    movea.l _DOSBase,a6
    jsr     LVOCurrentDir(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 132 $ff7c -$0084 IoErr()() */
    .globl _IoErr
_IoErr:
    link a5, #0
    move.l a6,-(sp)

    movea.l _DOSBase,a6
    jsr     LVOIoErr(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 138 $ff76 -$008a CreateProc(name,pri,segList,stackSize)(d1/d2/d3/d4) */
    .globl _CreateProc
_CreateProc:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* name */
    move.l  12(a5),d2 /* pri */
    move.l  16(a5),d3 /* segList */
    move.l  20(a5),d4 /* stackSize */
    movea.l _DOSBase,a6
    jsr     LVOCreateProc(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 144 $ff70 -$0090 Exit(returnCode)(d1) */
    .globl _Exit
_Exit:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* returnCode */
    movea.l _DOSBase,a6
    jsr     LVOExit(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 150 $ff6a -$0096 LoadSeg(name)(d1) */
    .globl _LoadSeg
_LoadSeg:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* name */
    movea.l _DOSBase,a6
    jsr     LVOLoadSeg(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 156 $ff64 -$009c UnLoadSeg(seglist)(d1) */
    .globl _UnLoadSeg
_UnLoadSeg:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* seglist */
    movea.l _DOSBase,a6
    jsr     LVOUnLoadSeg(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 174 $ff52 -$00ae DeviceProc(name)(d1) */
    .globl _DeviceProc
_DeviceProc:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* name */
    movea.l _DOSBase,a6
    jsr     LVODeviceProc(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 180 $ff4c -$00b4 SetComment(name,comment)(d1/d2) */
    .globl _SetComment
_SetComment:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* name */
    move.l  12(a5),d2 /* comment */
    movea.l _DOSBase,a6
    jsr     LVOSetComment(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 186 $ff46 -$00ba SetProtection(name,protect)(d1/d2) */
    .globl _SetProtection
_SetProtection:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* name */
    move.l  12(a5),d2 /* protect */
    movea.l _DOSBase,a6
    jsr     LVOSetProtection(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 192 $ff40 -$00c0 DateStamp(date)(d1) */
    .globl _DateStamp
_DateStamp:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* date */
    movea.l _DOSBase,a6
    jsr     LVODateStamp(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 198 $ff3a -$00c6 Delay(timeout)(d1) */
    .globl _Delay
_Delay:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* timeout */
    movea.l _DOSBase,a6
    jsr     LVODelay(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 204 $ff34 -$00cc WaitForChar(file,timeout)(d1/d2) */
    .globl _WaitForChar
_WaitForChar:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* file */
    move.l  12(a5),d2 /* timeout */
    movea.l _DOSBase,a6
    jsr     LVOWaitForChar(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 210 $ff2e -$00d2 ParentDir(lock)(d1) */
    .globl _ParentDir
_ParentDir:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* lock */
    movea.l _DOSBase,a6
    jsr     LVOParentDir(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 216 $ff28 -$00d8 IsInteractive(file)(d1) */
    .globl _IsInteractive
_IsInteractive:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* file */
    movea.l _DOSBase,a6
    jsr     LVOIsInteractive(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 222 $ff22 -$00de Execute(string,file,file2)(d1/d2/d3) */
    .globl _Execute
_Execute:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* string */
    move.l  12(a5),d2 /* file */
    move.l  16(a5),d3 /* file2 */
    movea.l _DOSBase,a6
    jsr     LVOExecute(a6)

    move.l (sp)+,a6
    unlk a5
    rts

