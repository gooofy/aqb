#include <assert.h>

#include "run.h"
#include "logger.h"
#include "util.h"

#ifdef __amigaos__
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <inline/exec.h>
#include <inline/dos.h>
extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;
#endif

#define DEFAULT_PRI           0
#define DEFAULT_STACKSIZE 32768

void RUN_run (const char *binfn)
{
#ifdef __amigaos__
#if 0
    struct MsgPort *mp, *reply_mp;
    struct DosPacket *dp;
    //SIPTR Res1, Res2;
    BPTR seg;

	dp = AllocDosObject(DOS_STDPKT, NULL);

    reply_mp = CreateMsgPort();
    seg = CreateSegList(internalBootCliHandler);
    if (dp == NULL || reply_mp == NULL || seg == BNULL) {
        if (my_dp)
            FreeDosObject(DOS_STDPKT, my_dp);
        DeleteMsgPort(reply_mp);
        UnLoadSeg(seg);
        return ERROR_NO_FREE_STORE;
    }

    mp = CreateProc("Boot Mount", 0, seg, AROS_STACKSIZE);
    if (mp == NULL) {
        DeleteMsgPort(reply_mp);
        if (my_dp)
            FreeDosObject(DOS_STDPKT, my_dp);
        /* A best guess... */
        UnLoadSeg(seg);
        return ERROR_NO_FREE_STORE;
    }

    /* Preload the reply with failure */
    dp->dp_Res1 = DOSFALSE;
    dp->dp_Res2 = ERROR_NOT_IMPLEMENTED;

    /* Again, doesn't require this Task to be a Process */
    SendPkt(dp, mp, reply_mp);

    /* Wait for the message from the Boot Cli */
    WaitPort(reply_mp);
    GetMsg(reply_mp);

    /* We know that if we've received a reply packet,
     * that we've been able to execute the handler,
     * therefore we can dispense with the 'CreateSegList()'
     * stub.
     */
    UnLoadSeg(seg);

    Res1 = dp->dp_Res1;
    Res2 = dp->dp_Res2;

    if (my_dp)
        FreeDosObject(DOS_STDPKT, my_dp);

    DeleteMsgPort(reply_mp);

    D(bug("Dos/CliInit: Process returned Res1=%ld, Res2=%ld\n", Res1, Res2));



#endif




    LOG_printf (LOG_INFO, "loading %s ...\n\n", binfn);
    BPTR seglist = LoadSeg((STRPTR)binfn);

    if (!seglist)
    {
        LOG_printf (LOG_ERROR, "failed to load %s\n\n", binfn);
        return;
    }

    LOG_printf (LOG_INFO, "running %s ...\n\n", binfn);
    struct MsgPort *msgport = CreateProc((STRPTR) binfn, DEFAULT_PRI, seglist, DEFAULT_STACKSIZE);
    assert(msgport);

    struct Message *xy_msg = WaitPort(msgport);
    assert (xy_msg);
    LOG_printf (LOG_INFO, "got a message\n\n");

    //UnLoadSeg(seglist);

#else // no amigaos -> posix / vamos?

    assert(FALSE); // FIXME

#endif
}

