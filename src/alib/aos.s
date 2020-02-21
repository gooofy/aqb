
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

/* dos */
LVODelay         = -198

    .globl  _OpenLibrary
_OpenLibrary:
    link a5, #0

    move.l  8(a5),a1        /* libName      */
    move.l 12(a5),d0        /* version      */
    movea.l SysBase,a6      /* exec base    */
    jsr     LVOOpenLibrary(a6)

    unlk a5
    rts

    .globl _CloseLibrary
_CloseLibrary:
    link a5, #0

    move.l  8(a5),a1        /* library      */
    movea.l SysBase,a6      /* exec base    */
    jsr     LVOCloseLibrary(a6)

    unlk a5
    rts

    .globl _CopyMem
_CopyMem:
    link a5, #0

    move.l  8(a5),a0        /* source       */
    move.l 12(a5),a1        /* dest         */
    move.l 16(a5),d0        /* size         */
    movea.l SysBase,a6      /* exec base    */
    jsr     LVOCopyMem(a6)

    unlk a5
    rts

    .globl _OpenWindow
_OpenWindow:
    link a5, #0

    move.l  8(a5),a0            /* newWindow         */
    movea.l _IntuitionBase,a6   /* intuition base    */
    jsr     LVOOpenWindow(a6)

    unlk a5
    rts

    .globl _CloseWindow
_CloseWindow:
    link a5, #0

    move.l  8(a5),a0            /* window            */
    movea.l _IntuitionBase,a6   /* intuition base    */
    jsr     LVOCloseWindow(a6)

    unlk a5
    rts

    .globl _AllocRemember
_AllocRemember:
    link a5, #0

    move.l  8(a5),a0            /* rememberKey       */
    move.l 12(a5),d0            /* size              */
    move.l 16(a5),d1            /* flags             */
    movea.l _IntuitionBase,a6   /* intuition base    */
    jsr     LVOAllocRemember(a6)

    unlk a5
    rts

    .globl _FreeRemember
_FreeRemember:
    link a5, #0

    move.l  8(a5),a0            /* rememberKey       */
    move.l 12(a5),d0            /* reallyForget      */
    movea.l _IntuitionBase,a6   /* intuition base    */
    jsr     LVOFreeRemember(a6)

    unlk a5
    rts

    .globl _Delay
_Delay:
    link a5, #0

    move.l  8(a5),d1            /* ticks             */
    movea.l _DOSBase,a6         /* dos base          */
    jsr     LVODelay(a6)

    unlk a5
    rts

