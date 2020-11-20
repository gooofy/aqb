#include "_brt.h"

#include <exec/types.h>
#include <clib/exec_protos.h>
#include <inline/exec.h>

// DATA / READ / RESTORE support

static void *g_data_ptr=NULL;

void _aqb_restore (void *p)
{
    g_data_ptr = p;
}

void _aqb_read (void *v, UWORD size)
{
    if (!g_data_ptr)
    {
        ERROR (ERR_OUT_OF_DATA);
        return;
    }
    CopyMem(g_data_ptr, v, size);
    g_data_ptr += size;
}

