#include "../_aqb/_aqb.h"
#include "../_brt/_brt.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <clib/intuition_protos.h>
#include <inline/intuition.h>

#include <clib/gadtools_protos.h>
#include <inline/gadtools.h>
#include <libraries/gadtools.h>

#include <clib/dos_protos.h>
#include <inline/dos.h>

struct Library           *GadToolsBase  = NULL;

static void _OSGadTools_shutdown(void)
{
    DPRINTF ("_OSGadTools_shutdown called\n");

    if (GadToolsBase)
    {
        DPRINTF ("_OSGadTools_shutdown CloseLibrary (GadToolsBase)\n");
        CloseLibrary (GadToolsBase);
    }

    DPRINTF ("_OSGadTools_shutdown done\n");
}

void _OSGadTools_init(void)
{
    if (!(GadToolsBase = OpenLibrary((CONST_STRPTR) "gadtools.library", 0)))
        _cshutdown(20, (UBYTE *)"*** error: failed to open gadtools.library!\n");

    ON_EXIT_CALL(_OSGadTools_shutdown);
}

