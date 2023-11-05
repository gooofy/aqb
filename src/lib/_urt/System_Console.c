#include "_urt.h"

#include <clib/dos_protos.h>
#include <inline/dos.h>

static BPTR g_stdout;

VOID _ZN6System7Console9__gc_scanEPN6System2GCEE (System_Console *this, System_GC *gc)
{
    _ACS_ASSERT (FALSE, (STRPTR) "FIXME: implement: System_Console gc_scan");
}

static char *_nl = "\n";

VOID _ZN6System7Console9WriteLineERN6System6StringEE (System_String *sstr)
{
    Write(g_stdout, (CONST APTR) sstr->_str, sstr->_len);
    Write(g_stdout, (CONST APTR) _nl, 1);
}

VOID _ZN6System7Console5WriteERN6System6StringEE (System_String *sstr)
{
    Write(g_stdout, (CONST APTR) sstr->_str, sstr->_len);
}

VOID _ZN6System7Console5WriteEiE (LONG value)
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

