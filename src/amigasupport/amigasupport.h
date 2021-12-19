#ifndef HAVE_AMIGA_SUPPORT_H
#define HAVE_AMIGA_SUPPORT_H

#include <exec/types.h>
#include <dos/dos.h>
#include <intuition/classusr.h>

// stuff from amigalib

struct MsgPort   *ASUP_create_port   (STRPTR name, LONG pri);
void              ASUP_delete_port   (struct MsgPort *port);
struct IORequest *ASUP_create_ext_io (CONST struct MsgPort * port, LONG io_size);
void              ASUP_delete_ext_io (struct IORequest * io);

// post-1.3 stuff

#define ASUP_DoPkt5(res2, port, action, arg1, arg2, arg3, arg4, arg5) ASUP_DoPkt(res2, port, action, (LONG)(arg1), (LONG)(arg2), (LONG)(arg3), (LONG)(arg4), (LONG)(arg5), 0, 0)
#define ASUP_DoPkt4(res2, port, action, arg1, arg2, arg3, arg4)       ASUP_DoPkt(res2, port, action, (LONG)(arg1), (LONG)(arg2), (LONG)(arg3), (LONG)(arg4), 0, 0, 0)
#define ASUP_DoPkt3(res2, port, action, arg1, arg2, arg3)             ASUP_DoPkt(res2, port, action, (LONG)(arg1), (LONG)(arg2), (LONG)(arg3), 0, 0, 0, 0)
#define ASUP_DoPkt2(res2, port, action, arg1, arg2)                   ASUP_DoPkt(res2, port, action, (LONG)(arg1), (LONG)(arg2), 0, 0, 0, 0, 0)
#define ASUP_DoPkt1(res2, port, action, arg1)                         ASUP_DoPkt(res2, port, action, (LONG)(arg1), 0, 0, 0, 0, 0, 0)
#define ASUP_DoPkt0(res2, port, action)                               ASUP_DoPkt(res2, port, action, 0, 0, 0, 0, 0, 0, 0)

struct DosPacket *ASUP_AllocDosPacket (void);
void              ASUP_SendPkt        (struct DosPacket *dp, struct MsgPort *port, struct MsgPort *replyport);
struct DosPacket *ASUP_WaitPkt        (struct MsgPort *msgPort);
void              ASUP_FreePkt        (struct DosPacket *dp);
LONG              ASUP_DoPkt          (LONG *res2, struct MsgPort *port, LONG action, LONG arg1, LONG arg2, LONG arg3, LONG arg4, LONG arg5, LONG arg6, LONG arg7);
BOOL              ASUP_NameFromLock   (BPTR lock, STRPTR buffer, LONG len);

ULONG             ASUP_CallHookA      (struct Hook *, Object *, APTR );
ULONG             ASUP_DoMethodA      (Object * obj,Msg msg);
ULONG             ASUP_DoMethod       (Object *obj, ULONG method_id, ...);

#endif

