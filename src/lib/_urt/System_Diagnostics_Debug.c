#define ENABLE_DPRINTF
#include "_urt.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>

#include <inline/exec.h>

VOID _ZN6System11Diagnostics5Debug9__gc_scanEPN6System2GCE (System_Diagnostics_Debug *this, System_GC *gc)
{
    _ACS_ASSERT (FALSE, (STRPTR) "FIXME: implement: System_Diagnostics_Debug gc_scan");
}

VOID _ZN6System11Diagnostics5Debug6AssertEbE (BOOL condition, System_String *compiler_msg)
{
    _ACS_ASSERT (condition, (STRPTR) compiler_msg->_str);
}
