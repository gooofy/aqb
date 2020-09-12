#include "_brt.h"
#include <stdarg.h>

#include <exec/memory.h>

#include <clib/exec_protos.h>

#include <inline/exec.h>

_dyna *_dyna_create(UWORD numDims, ULONG elementSize, ...)
{
    va_list valist;
    va_start (valist, elementSize);
    for (UWORD iDim=0; iDim<numDims; iDim++)
    {
        _debug_puts ("_dyna_create: dim\n");
    }

    va_end(valist);

    return NULL; // FIXME
}

