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

    #define SysBase 4

    /* exec.library */

    #define FindTask -294
    #define WaitPort -384
    #define GetMsg -372
    #define ReplyMsg -378
    #define Forbid -132

    /* structs */

/*    .set    pr_CLI, 172
    .set    pr_MsgPort, 92*/

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
	tst.l    172(a2)
	/*tst.l    pr_CLI(a2) */
	bne.s    NoWorkbench

	/* Receive Workbench startup message */

	lea		 92(a2), a0	  /* process message port   */
	/*lea		 pr_MsgPort(a2), a0*/	  /* process message port   */
	jsr      WaitPort(a6)         /* first wait for message */
	lea      92(a2), a0	  /* A0 is scratch, could be overwritten */
	/*lea      pr_MsgPort(a2), a0 */	  /* A0 is scratch, could be overwritten */
	jsr	     GetMsg(a6)           /* then get message from port */
	move.l	 d0, ___WorkbenchMsg  /* store it for later use */

NoWorkbench:

	/* main program entry here */
    jsr      __cstartup

    jsr      __aqb_main

    move.l   #0, -(sp)       /* 0 return code in case we reach this point (i. e. _aqb_main hasn't called _autil_exit(rc)) */
    jsr      __autil_exit


    .globl __autil_exit
__autil_exit:

    jsr      __c_atexit

	/* if in Workbench mode, reply the startup message now. */

	move.l	 ___WorkbenchMsg, d2
	tst.l	 d2				 /* check if we've got the message */
	beq.s	 NoReplyNeeded
	jsr      Forbid(a6)		 /* disable multitasking */
	movea.l	 d2, a1
	jsr		 ReplyMsg(a6)	 /* reply the message */

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

___WorkbenchMsg:
	dc.l	0

