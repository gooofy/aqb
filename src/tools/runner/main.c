
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/interrupts.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>

#include <devices/input.h>
#include <devices/inputevent.h>

#include "run.h"
#include "logger.h"
#include "amigasupport.h"

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;

#define BINFN "SYS:x/foo"
//#define BINFN "SYS:utilities/clock"

static struct MsgPort  *g_debugPort;

void __chkabort(void)
{
}

int main (int argc, char *argv[])
{
    g_debugPort  = ASUP_create_port ((STRPTR) "AQB debug reply port", 0);
    if (!g_debugPort)
    {
        // FIXME
        fprintf (stderr, "run: failed to create debug reply port!\n");
        exit(42);
    }

    BPTR o = Output();
    struct FileHandle *output = (struct FileHandle *) BADDR(o);

    RUN_init (g_debugPort, output);

    LOG_printf (LOG_DEBUG, "main: runner starts:\n");
    RUN_start (BINFN);

    ULONG debugsig  = 1 << g_debugPort->mp_SigBit;
    //ULONG debugsig  = g_debugPort->mp_SigBit;

    BOOL running = TRUE;
    while (running)
    {
        LOG_printf (LOG_DEBUG, "main: waiting for signals... 0x%08lx\n", debugsig);
        ULONG signals = Wait(debugsig);

        if (signals & debugsig)
        {
            LOG_printf (LOG_DEBUG, "got debugsignal\n");
            RUN_handleMessages();
            switch (RUN_getState())
            {
                case RUN_stateRunning:
                    break;
                case RUN_stateStopped:
                    running = FALSE;
                    break;
            }
        }
    }

    LOG_printf (LOG_DEBUG, "runner ends\n");

    ASUP_delete_port(g_debugPort);

    return 0;
}

