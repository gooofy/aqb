/*
 * startup code for AQB programs
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

    .set SysBase, 4

    /* exec.library */

    .set FindTask, -294
    .set WaitPort, -384
    .set GetMsg  , -372
    .set ReplyMsg, -378
    .set Forbid  , -132

    /* structs */

    .set pr_CLI    , 172
    .set pr_MsgPort,  92

    .set dbg_sig,     24
    .set dbg_code,    30

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
	move.l	 d0, ___StartupMsg  /* store it for later use */

runMain:

	/* main program entry here */
    jsr      __cstartup

    jsr      __aqb_main

    move.l   #0, -(sp)       /* 0 return code in case we reach this point (i. e. _aqb_main hasn't called _autil_exit(rc)) */
    jsr      __autil_exit


    .globl __autil_exit
__autil_exit:

    jsr      __c_atexit

	/* if in Workbench mode, reply to the startup message now. */

	move.l	 ___StartupMsg, d2
	tst.l	 d2				 /* check if we've got the message */
	beq.s	 NoReplyNeeded

    move.l   d2, a0

    /* was this in fact a debug message from the AQB IDE ? */
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
