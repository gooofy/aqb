#include "_urt.h"

#include <clib/dos_protos.h>
#include <inline/dos.h>

static BPTR g_stdout;

VOID _System_Console___gc_scan (System_Console *this, System_GC *gc)
{
    _ACS_ASSERT (FALSE, (STRPTR) "FIXME: implement: System_Console gc_scan");
}

static char *_nl = "\n";

VOID _System_Console_WriteLine (System_String *sstr)
{
    Write(g_stdout, (CONST APTR) sstr->_str, sstr->_len);
    Write(g_stdout, (CONST APTR) _nl, 1);
}

VOID _System_Console_Write (System_String *sstr)
{
    Write(g_stdout, (CONST APTR) sstr->_str, sstr->_len);
}

VOID _System_Console_WriteInt (LONG value)
{
    char buf[20];
    _astr_itoa_ext (value, (UBYTE*) buf, /*base=*/10, /*leading_space=*/FALSE, /*positive_sign=*/FALSE);
    Write(g_stdout, (CONST APTR) buf, _astr_len((UBYTE*)buf));
}

// FIXME: static initializer
void _console_init (void)
{
    g_stdout = Output();
}

void _console_shutdown (void)
{
}

