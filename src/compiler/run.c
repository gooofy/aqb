#include <assert.h>

#include "run.h"
#include "logger.h"
#include "util.h"

#ifdef __amigaos__
#include <clib/dos_protos.h>
#include <inline/dos.h>
extern struct DOSBase       *DOSBase;
#endif

void RUN_run (const char *binfn)
{
#ifdef __amigaos__

    LOG_printf (LOG_INFO, "loading %s ...\n\n", binfn);
    BPTR seglist = LoadSeg((STRPTR)binfn);

    //LOG_printf (LOG_INFO, "running %s ...\n\n", binfn);
    //struct MsgPort *CreateProc(STRPTR, LONG, BPTR, LONG)

    UnLoadSeg(seglist);

#else // no amigaos -> posix / vamos?

    assert(FALSE); // FIXME

#endif
}

