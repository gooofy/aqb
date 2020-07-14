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

    move.l   sp,___SaveSP    /* save sp for exit() at any point in the program */

    jsr     __cstartup

    .globl __autil_exit
__autil_exit:

    jsr     __c_atexit

    move.l  4(sp), d0       /* return code */
    move.l   ___SaveSP,sp
    rts

.data
    .align 4
___SaveSP:
    dc.l    0
