
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

static int g_termSignalBit  =-1;
static int g_debugSignalBit =-1;

void __chkabort(void)
{
}

static APTR ctrlCCallback ( register struct InputEvent *oldEventChain __asm("a0"),
                            register APTR               data          __asm("a1"))
{
    //LOG_printf (LOG_INFO, "ctrlCCallback called: oldEventChain=0x%08lx data=%ld\n", (ULONG) oldEventChain, (ULONG) data);

    struct InputEvent *e = oldEventChain;
    while (e)
    {
        if ( (e->ie_Class == IECLASS_RAWKEY) && ((e->ie_Code & 0x7f) == 0x33) && (e->ie_Qualifier & IEQUALIFIER_CONTROL) )
        {
            struct Task *maintask = data;
            Signal (maintask, 1<<g_debugSignalBit);
        }

        e = e->ie_NextEvent;
    }

	return oldEventChain;
}

static char *ctrlCHandlerName = "AQB CTRL-C input event handler";

int main (int argc, char *argv[])
{

    g_termSignalBit = AllocSignal(-1);
    if (g_termSignalBit == -1)
    {
		LOG_printf (LOG_ERROR, "failed to alloc signal\n");
        exit(23);
    }

    g_debugSignalBit = AllocSignal(-1);
    if (g_debugSignalBit == -1)
    {
		LOG_printf (LOG_ERROR, "failed to alloc signal\n");
        exit(24);
    }

    BPTR o = Output();
    struct FileHandle *output = (struct FileHandle *) BADDR(o);

    RUN_init (g_termSignalBit, output);

	struct IOStdReq  *inputReqBlk;
	struct MsgPort   *inputPort;
	struct Interrupt *inputHandler;

	if ( (inputPort=ASUP_create_port(NULL, 0)) )
	{
		if ( (inputHandler=AllocMem(sizeof(struct Interrupt), MEMF_PUBLIC|MEMF_CLEAR)) )
		{
			if ( (inputReqBlk=(struct IOStdReq *)ASUP_create_ext_io(inputPort, sizeof(struct IOStdReq))) )
			{
				if (!OpenDevice ((STRPTR)"input.device", /*unitNumber=*/0, (struct IORequest *)inputReqBlk, /*flags=*/0))
				{
					inputHandler->is_Code         = (APTR) ctrlCCallback;
					inputHandler->is_Data         = (APTR) FindTask(NULL);
					inputHandler->is_Node.ln_Pri  = 100;
					inputHandler->is_Node.ln_Name = ctrlCHandlerName;

					inputReqBlk->io_Data    = (APTR)inputHandler;
					inputReqBlk->io_Command = IND_ADDHANDLER;

					DoIO((struct IORequest *)inputReqBlk);

					LOG_printf (LOG_DEBUG, "runner starts:\n");
					RUN_start (BINFN);

					ULONG termsig  = 1 << g_termSignalBit;
					ULONG debugsig = 1 << g_debugSignalBit;

					BOOL running = TRUE;
					while (running)
					{
						ULONG signals = Wait(termsig | debugsig);

						if (signals & termsig)
						{
							LOG_printf (LOG_DEBUG, "got termsignal\n");
							running = FALSE;
						}

                        if (signals & debugsig)
                        {
							LOG_printf (LOG_DEBUG, "got debugsignal\n");
                            RUN_stop ();
                        }
					}

					LOG_printf (LOG_DEBUG, "runner ends\n");

					inputReqBlk->io_Data    = (APTR)inputHandler;
					inputReqBlk->io_Command = IND_REMHANDLER;

					DoIO((struct IORequest *)inputReqBlk);

					CloseDevice((struct IORequest *)inputReqBlk);
				}
				else
					printf("Error: Could not open input.device\n");

				ASUP_delete_ext_io((struct IORequest *)inputReqBlk);
			}
			else
				printf("Error: Could not create IORequest\n");

			FreeMem(inputHandler,sizeof(struct Interrupt));
		}
		else
			printf("Error: Could not allocate interrupt struct memory\n");

		ASUP_delete_port(inputPort);
	}
	else
		printf("Error: Could not create message port\n");


    FreeSignal (g_termSignalBit);

    return 0;
}

