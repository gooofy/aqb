/* this is only a minimum subset of the AmigaOS - just what we need to support the core AQB language */

/*
 * exec
 */

_SysBase          = 4

LVOAllocMem       = -198
LVOFreeMem        = -210
LVOCloseLibrary   = -414
LVOOpenLibrary    = -552
LVOCopyMem        = -624

/* 198 $ff3a -$00c6 AllocMem(byteSize,requirements)(d0/d1) */
    .globl _AllocMem
_AllocMem:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* byteSize */
    move.l  12(a5),d1 /* requirements */
    movea.l _SysBase,a6
    jsr     LVOAllocMem(a6)

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
    movea.l _SysBase,a6
    jsr     LVOFreeMem(a6)

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
    movea.l _SysBase,a6
    jsr     LVOCopyMem(a6)

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
    movea.l _SysBase,a6
    jsr     LVOOpenLibrary(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 414 $fe62 -$019e CloseLibrary(library)(a1) */
    .globl _CloseLibrary
_CloseLibrary:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),a1 /* library */
    movea.l _SysBase,a6
    jsr     LVOCloseLibrary(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/*
 * DOS
 */

LVOWrite                   = -48
LVOOutput                  = -60
LVODateStamp               = -192

/* 48 $ffd0 -$0030 Write(file,buffer,length)(d1/d2/d3) */
    .globl _Write
_Write:
    link a5, #0
    move.l a6,-(sp)
    move.l d2,-(sp)
    move.l d3,-(sp)

    move.l  8(a5),d1 /* file */
    move.l  12(a5),d2 /* buffer */
    move.l  16(a5),d3 /* length */
    movea.l _DOSBase,a6
    jsr     LVOWrite(a6)

    move.l (sp)+,d3
    move.l (sp)+,d2
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

/*
 * FFP
 */

LVOSPFix                   = -30
LVOSPFlt                   = -36
LVOSPCmp                   = -42
LVOSPTst                   = -48
LVOSPAbs                   = -54
LVOSPNeg                   = -60
LVOSPAdd                   = -66
LVOSPSub                   = -72
LVOSPMul                   = -78
LVOSPDiv                   = -84
LVOSPFloor                 = -90
LVOSPCeil                  = -96
/* 30 $ffe2 -$001e SPFix(parm)(d0) */
    .globl _SPFix
_SPFix:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathBase,a6
    jsr     LVOSPFix(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 36 $ffdc -$0024 SPFlt(integer)(d0) */
    .globl _SPFlt
_SPFlt:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* integer */
    movea.l _MathBase,a6
    jsr     LVOSPFlt(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 42 $ffd6 -$002a SPCmp(leftParm,rightParm)(d0,d1) */
    .globl _SPCmp
_SPCmp:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* leftParm */
    move.l  12(a5),d1 /* rightParm */
    movea.l _MathBase,a6
    jsr     LVOSPCmp(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 48 $ffd0 -$0030 SPTst(parm)(d1) */
    .globl _SPTst
_SPTst:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* parm */
    movea.l _MathBase,a6
    jsr     LVOSPTst(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 54 $ffca -$0036 SPAbs(parm)(d0) */
    .globl _SPAbs
_SPAbs:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathBase,a6
    jsr     LVOSPAbs(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 60 $ffc4 -$003c SPNeg(parm)(d0) */
    .globl _SPNeg
_SPNeg:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathBase,a6
    jsr     LVOSPNeg(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 66 $ffbe -$0042 SPAdd(leftParm,rightParm)(d1,d0) */
    .globl _SPAdd
_SPAdd:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* leftParm */
    move.l  12(a5),d0 /* rightParm */
    movea.l _MathBase,a6
    jsr     LVOSPAdd(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 72 $ffb8 -$0048 SPSub(leftParm,rightParm)(d1,d0)  = rightParm - leftParm*/
    .globl _SPSub
_SPSub:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* leftParm */
    move.l  12(a5),d0 /* rightParm */
    movea.l _MathBase,a6
    jsr     LVOSPSub(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 78 $ffb2 -$004e SPMul(leftParm,rightParm)(d1,d0) */
    .globl _SPMul
_SPMul:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* leftParm */
    move.l  12(a5),d0 /* rightParm */
    movea.l _MathBase,a6
    jsr     LVOSPMul(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 84 $ffac -$0054 SPDiv(leftParm,rightParm)(d1,d0) */
    .globl _SPDiv
_SPDiv:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d1 /* leftParm */
    move.l  12(a5),d0 /* rightParm */
    movea.l _MathBase,a6
    jsr     LVOSPDiv(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 90 $ffa6 -$005a SPFloor(parm)(d0) */
    .globl _SPFloor
_SPFloor:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathBase,a6
    jsr     LVOSPFloor(a6)

    move.l (sp)+,a6
    unlk a5
    rts

/* 96 $ffa0 -$0060 SPCeil(parm)(d0) */
    .globl _SPCeil
_SPCeil:
    link a5, #0
    move.l a6,-(sp)

    move.l  8(a5),d0 /* parm */
    movea.l _MathBase,a6
    jsr     LVOSPCeil(a6)

    move.l (sp)+,a6
    unlk a5
    rts

LVOSPAtan                  = -30
LVOSPSin                   = -36
LVOSPCos                   = -42
LVOSPTan                   = -48
LVOSPSincos                = -54
LVOSPSinh                  = -60
LVOSPCosh                  = -66
LVOSPTanh                  = -72
LVOSPExp                   = -78
LVOSPLog                   = -84
LVOSPPow                   = -90
LVOSPSqrt                  = -96
LVOSPTieee                 = -102
LVOSPFieee                 = -108
LVOSPAsin                  = -114
LVOSPAcos                  = -120
LVOSPLog10                 = -126
/* 30 $ffe2 -$001e SPAtan(parm)(d0) */
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

/* 36 $ffdc -$0024 SPSin(parm)(d0) */
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

/* 42 $ffd6 -$002a SPCos(parm)(d0) */
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

/* 48 $ffd0 -$0030 SPTan(parm)(d0) */
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

/* 54 $ffca -$0036 SPSincos(cosResult,parm)(d1,d0) */
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

/* 60 $ffc4 -$003c SPSinh(parm)(d0) */
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

/* 66 $ffbe -$0042 SPCosh(parm)(d0) */
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

/* 72 $ffb8 -$0048 SPTanh(parm)(d0) */
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

/* 78 $ffb2 -$004e SPExp(parm)(d0) */
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

/* 84 $ffac -$0054 SPLog(parm)(d0) */
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

/* 90 $ffa6 -$005a SPPow(power,arg)(d1,d0) = arg^power */
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

/* 96 $ffa0 -$0060 SPSqrt(parm)(d0) */
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

/* 102 $ff9a -$0066 SPTieee(parm)(d0) */
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

/* 108 $ff94 -$006c SPFieee(parm)(d0) */
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

/* 114 $ff8e -$0072 SPAsin(parm)(d0) */
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

/* 120 $ff88 -$0078 SPAcos(parm)(d0) */
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

/* 126 $ff82 -$007e SPLog10(parm)(d0) */
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



