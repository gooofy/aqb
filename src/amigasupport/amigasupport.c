#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "amigasupport.h"

#include <exec/memory.h>
#include <exec/execbase.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>
#include <inline/intuition.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;

#define NEWLIST(l) ((l)->lh_Head = (struct Node *)&(l)->lh_Tail, \
                    /*(l)->lh_Tail = NULL,*/ \
                    (l)->lh_TailPred = (struct Node *)&(l)->lh_Head)

struct MsgPort *ASUP_create_port(STRPTR name, LONG pri)
{
    struct MsgPort *port = NULL;
    UBYTE portsig;

    if ((BYTE)(portsig=AllocSignal(-1)) >= 0)
    {
        if (!(port=AllocMem(sizeof(*port),MEMF_CLEAR|MEMF_PUBLIC)))
        {
            FreeSignal(portsig);
        }
        else
        {
            port->mp_Node.ln_Type = NT_MSGPORT;
            port->mp_Node.ln_Pri  = pri;
            port->mp_Node.ln_Name = (char *)name;
            /* done via AllocMem
            port->mp_Flags        = PA_SIGNAL;
            */
            port->mp_SigBit       = portsig;
            port->mp_SigTask      = FindTask(NULL);
            NEWLIST(&port->mp_MsgList);
            if (port->mp_Node.ln_Name)
                AddPort(port);
        }
    }
    return port;
}

void ASUP_delete_port(struct MsgPort *port)
{
    if (port->mp_Node.ln_Name)
        RemPort(port);
    FreeSignal(port->mp_SigBit);
    FreeMem(port,sizeof(*port));
}

struct DosPacket *ASUP_AllocDosPacket(void)
{
    struct StandardPacket *sp = AllocVec(sizeof(struct StandardPacket), MEMF_CLEAR);

    if (sp == NULL)
        return NULL;

    sp->sp_Pkt.dp_Link         = &sp->sp_Msg;
    sp->sp_Msg.mn_Node.ln_Name = (char *)&sp->sp_Pkt;

    return &sp->sp_Pkt;
}

void ASUP_SendPkt(struct DosPacket *dp, struct MsgPort *port, struct MsgPort *replyport)
{
    dp->dp_Port               = replyport;
    dp->dp_Link->mn_ReplyPort = replyport;

    PutMsg(port, dp->dp_Link);
}

struct DosPacket *ASUP_WaitPkt(struct MsgPort *msgPort)
{
    struct Message   *msg = NULL;

	while ((msg = GetMsg(msgPort)) == NULL)
	{
		Wait(1 << msgPort->mp_SigBit);
	}

    return (struct DosPacket *)msg->mn_Node.ln_Name;
}

void ASUP_FreePkt(struct DosPacket *dp)
{
	BYTE *p = (BYTE*) dp;
	p -= offsetof (struct StandardPacket, sp_Pkt);

    FreeVec(p);
}

LONG ASUP_DoPkt(LONG *res2, struct MsgPort *port, LONG action, LONG arg1, LONG arg2, LONG arg3, LONG arg4, LONG arg5, LONG arg6, LONG arg7)
{
    struct Process *me = (struct Process *)FindTask(NULL);

    if (port == NULL)
		return FALSE;

    struct DosPacket *dp = ASUP_AllocDosPacket();
    if (!dp)
        return FALSE;

    struct MsgPort *replyPort = &me->pr_MsgPort;

    dp->dp_Type = action;
    dp->dp_Arg1 = arg1;
    dp->dp_Arg2 = arg2;
    dp->dp_Arg3 = arg3;
    dp->dp_Arg4 = arg4;
    dp->dp_Arg5 = arg5;
    dp->dp_Arg6 = arg6;
    dp->dp_Arg7 = arg7;
    dp->dp_Res1 = 0;
    dp->dp_Res2 = 0;

    ASUP_SendPkt(dp, port, replyPort);

    dp = ASUP_WaitPkt(replyPort);

    LONG res = dp->dp_Res1;
    if (res2)
        *res2 = dp->dp_Res2;

    me->pr_Result2 = dp->dp_Res2;

    ASUP_FreePkt(dp);

    return res;
}

BOOL ASUP_NameFromLock(BPTR lock, STRPTR buffer, LONG length)
{
    if ((length <= 0) || !lock)
        return FALSE;

    struct FileInfoBlock *fib = (struct FileInfoBlock *) AllocMem(sizeof (struct FileInfoBlock), MEMF_CLEAR);
    if (!fib)
        return FALSE;

    BPTR mylock = DupLock(lock);
    if (!mylock)
        return FALSE;

    // we start at the end (to avoid recursion) and word back to front

    char *name  = (char *) &buffer[length-1];
    *name = '\0';

	BPTR mylock2;
	BOOL err = FALSE;
    while ((mylock2 = ParentDir(mylock)))
    {
        if (Examine (mylock, fib))
        {
			//printf ("ASUP_NameFromLock: fib->fib_FileName=%s\n", fib->fib_FileName);
            LONG len = strlen (fib->fib_FileName);
            if ((STRPTR)(name-(len+4)) < buffer)
            {
                UnLock(mylock2);
				err = TRUE;
                goto clean_exit;
            }

            if (*name)
                *--name = '/';

            CopyMem (fib->fib_FileName, name-len, len);
            name -= len;
        }
		else
		{
			err = TRUE;
            goto clean_exit;
        }
		UnLock (mylock);

        mylock = mylock2;
    }

    if (IoErr())
	{
		err = TRUE;
        goto clean_exit;
	}

	// prepend device name -> extract from lock
    *--name = ':';

	struct FileLock *fl = BADDR(mylock);
	struct DeviceList *dl = BADDR(fl->fl_Volume);
	char *device_name = BADDR(dl->dl_Name);
    LONG len = *device_name++;
	// printf ("ASUP_NameFromLock: device_name=%s\n", device_name);
    // for (int i=0; i<6; i++)
    //     printf ("%c [%d]\n", device_name[i], device_name[i]);

    if ((STRPTR)(name-len) < buffer)
    {
		err = TRUE;
        goto clean_exit;
    }
    strncpy ((char *)buffer, device_name, len);

	// move to front
    strcpy ((char *) (buffer+len), name);

clean_exit:
	UnLock (mylock);
    FreeMem (fib, sizeof (struct FileInfoBlock));

	return !err;
}

struct IORequest *ASUP_create_ext_io(CONST struct MsgPort * port, LONG io_size)
{
    struct IORequest * result = NULL;

    if (port == NULL || io_size < (LONG)sizeof(*result))
        goto out;

    result = CreateIORequest (port, (ULONG)io_size);

out:
    return result;
}

void ASUP_delete_ext_io(struct IORequest * io)
{
    if (io != NULL)
        DeleteIORequest((struct IORequest *)io);
}

asm("

h_Entry = 8

    .text
    .even

|---------------------------------------------------------------------------
| new hook standard
| use struct Hook (with minnode at the top)
|
| *** register calling convention: ***
|   A0 - pointer to hook itself
|   A1 - pointer to parameter packed ('message')
|   A2 - Hook specific address data ('object,' e.g, gadget )
|
| ***  C conventions: ***
| Note that parameters are in unusual register order: a0, a2, a1.
| This is to provide a performance boost for assembly language
| programming (the object in a2 is most frequently untouched).
| It is also no problem in 'register direct' C function parameters.
|
| calling through a hook
|   CallHook( hook, object, msgid, p1, p2, ... );
|   CallHookA( hook, object, msgpkt );
|
| using a C function:   CFunction( hook, object, message );
|   hook.h_Entry = HookEntry;
|   hook.h_SubEntry = CFunction;
|
|---------------------------------------------------------------------------

| C calling hook interface for prepared message packet

    .globl  _ASUP_CallHookA

_ASUP_CallHookA:

    moveml  a2/a6,sp@-
    moveal  sp@(12),a0
    moveal  sp@(16),a2
    moveal  sp@(20),a1
    pea     callhooka_return
    movel   a0@(h_Entry),sp@-
    rts

callhooka_return:

    moveml  sp@+,a2/a6
    rts

");

ULONG ASUP_DoMethodA(Object * obj,Msg msg)
{
    ULONG result = 0;

    if(obj != NULL)
    {
        Class * cl;

        cl = OCLASS(obj);

        if(cl != NULL)
            result = ASUP_CallHookA(&cl->cl_Dispatcher, obj, msg);
    }

    return result;
}

ULONG ASUP_DoMethod(Object *obj, ULONG method_id, ...)
{
    ULONG result = 0;

    if (obj != NULL)
        result = ASUP_DoMethodA(obj,(Msg)&method_id);

    return result;
}


