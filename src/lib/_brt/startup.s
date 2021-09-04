/*
 * startup code for AQB programs
 *
 * hands over control to c startup, provides _exit() function
 *
 * TODO: handle commandline, workbench startup
 */

.text
    .globl  _start
_start:

    /*
    movel   a0,___commandline
    movel   d0,___commandlen
    */

    /* save sp and all registers so we can restore them in __autil_exit called from any point in the program */
    movem.l  d2-d7/a2-a6, -(sp)
    move.l   sp,___SaveSP

    jsr     __cstartup

    jsr     __aqb_main

    move.l  #0, -(sp)       /* 0 return code in case we reach this point (i. e. _aqb_main hasn't called _autil_exit(rc)) */
    jsr     __autil_exit

    .globl __autil_exit
__autil_exit:

    jsr     __c_atexit

    move.l  4(sp), d0       /* return code */

    /* restore sp, registers */
    move.l   ___SaveSP,sp
    movem.l  (sp)+,d2-d7/a2-a6
    rts

.data
    .align 4
___SaveSP:
    dc.l    0

