#define ENABLE_DPRINTF
#include "_urt.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>

#include <inline/exec.h>

VOID _System_Diagnostics_Debug___gc_scan (System_Diagnostics_Debug *this, System_GC *gc)
{
    _ACS_ASSERT (FALSE, (STRPTR) "FIXME: implement: System_Diagnostics_Debug gc_scan");
}

VOID _System_Diagnostics_Debug_Assert (BOOL condition, System_String *compiler_msg)
{
    _ACS_ASSERT (condition, (STRPTR) compiler_msg->_str);
}
