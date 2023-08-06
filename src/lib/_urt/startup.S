/*
 * startup code for ACS programs
 *
 * hands over control to c startup, provides _exit() function
 *
 * based on
 *   Amiga programming example in assembler.
 *   Minimal Workbench startup code.
 *   Grzegorz Kraszewski, 2016
 *   Public Domain
 *   https://github.com/Sakura-IT/Amiga-programming-examples/blob/master/ASM/MiniStartup/startup.s
 *
 * TODO: handle commandline
 */


    #define SysBase 4

    /* exec.library */

    #define FindTask  -294
    #define WaitPort  -384
    #define GetMsg    -372
    #define ReplyMsg  -378
    #define Forbid    -132
    #define StackSwap -732

    /* structs */

    #define pr_CLI     172
    #define pr_MsgPort  92

    #define dbg_sig     24
    #define dbg_code    34
    #define dbg_exitFn  30

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

	/* Let's find our own process descriptor */

	suba.l	 a1, a1			/* NULL here means "find my own process" */
	movea.l	 SysBase, a6
	jsr      FindTask(A6)

	/* Check process console pointer. If 0 -> started from Workbench. */

	movea.l	 d0, a2			/* process descriptor in A2 */
	tst.l    pr_CLI(a2)
	bne.s    runMain

	/* Receive Workbench startup message */

	lea		 pr_MsgPort(a2), a0	  /* process message port   */
	jsr      WaitPort(a6)         /* first wait for message */
	lea      pr_MsgPort(a2), a0   /* A0 is scratch, could be overwritten */
	jsr	     GetMsg(a6)           /* then get message from port */
	move.l	 d0, ___StartupMsg    /* store it for later use */

    /* was this a dbg message sent by the AQB IDE ? */

    move.l   d0, a0
    move.l   dbg_sig(a0), d1      /* check signature */
    cmpi.l   #0xDECA11ED, d1
    bne.s    runMain              /* regular wb start message */

    /* this _is_ a debug message -> put exit function pointer into it */

    move.l   #__autil_exit, dbg_exitFn(a0)

runMain:

	/* main program entry here */
    jsr      __cstartup

    /* swap stack ? */

    move.l   __g_stack, d0
    beq.s    noStackSwap

    /* setup struct StackSwapStruct */
    lea      ___StackSwap, a0
	move.l   d0, (a0)+
	add.l    __acs_stack_size, d0
	move.l   d0, (a0)+
	moveq    #-16, d1
	add.l    d0, d1
	move.l   d1, (a0)

    /* call exec StackSwap() */
    lea      ___StackSwap, a0
	jsr      StackSwap(A6)

noStackSwap:

    jsr      __acs_main

    move.l   #0, -(sp)       /* 0 return code in case we reach this point (i. e. _acs_main hasn't called _autil_exit(rc)) */
    bsr      __autil_exit


    .globl __autil_exit
__autil_exit:

    /* restore stack in case we swapped it */
    move.l   __g_stack, d0
    beq.s    noStackRestore

    move.l   4(sp), d2       /* fetch return code from custom stack */

    lea      ___StackSwap, a0
	movea.l	 SysBase, a6
	jsr      StackSwap(A6)

    move.l   d2, -(sp)       /* push return code on os stack */
    move.l   d2, -(sp)       /* fake return addr (doesn't matter, we will reset sp down below anyway) */

noStackRestore:

    jsr      __c_atexit

	/* if in Workbench mode, reply to the startup message now. */

	move.l	 ___StartupMsg, d2
	tst.l	 d2				 /* check if we've got the message */
	beq.s	 NoReplyNeeded

    move.l   d2, a0

    /* was this in fact a debug message from the ACS IDE ? */
    move.l   dbg_sig(a0), d0      /* check signature */
    cmpi.l   #0xDECA11ED, d0
    bne.s    NoDbgMsg             /* regular wb start message */

    /* put error code in our reply message */
    move.w   _ERR, d0
    ext.l    d0
    move.l   d0, dbg_code(a0)

NoDbgMsg:
	movea.l	 SysBase, a6
	jsr      Forbid(a6)		 /* disable multitasking */
	movea.l	 d2, a1
	jsr		 ReplyMsg(a6)	 /* reply to the message */

NoReplyNeeded:
    move.l   4(sp), d0       /* return code */

    /* restore sp, registers */
    move.l   ___SaveSP,sp
    movem.l  (sp)+,d2-d7/a2-a6
    rts

.data
    .align 4
___SaveSP:
    dc.l    0

	.global ___StartupMsg
___StartupMsg:
	dc.l	0

___StackSwap:
	dc.l	0
	dc.l	0
	dc.l	0
