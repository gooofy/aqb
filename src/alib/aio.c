
#include "astr.h"
#include "aio.h"

#include <clib/dos_protos.h>

// LONG Write( BPTR file, CONST APTR buffer, LONG length );


#define MAXBUF 40

static BPTR g_stdout;

void _aio_init(void)
{
    g_stdout = Output();
}

void _aio_shutdown(void)
{
}

void _aio_puts(char *s)
{
    Write(g_stdout, (CONST APTR) s, _astr_len(s));
}

void _aio_putnl(void)
{
    _aio_puts("\n");
}

void _aio_puttab(void)
{
    _aio_puts("\t");
}

void _aio_puts4(int num)
{
    char buf[MAXBUF];

    if (num<0)
    {
        _aio_puts("-");
        num *= -1;
    }
    else
    {
        _aio_puts(" ");
    }

    _astr_itoa(num, buf, 10);

    _aio_puts(buf);
}

void _aio_puts2(short num)
{
    char buf[MAXBUF];

    if (num<0)
    {
        _aio_puts("-");
        num *= -1;
    }
    else
    {
        _aio_puts(" ");
    }

    _astr_itoa(num, buf, 10);

    _aio_puts(buf);
}

void _aio_puthex(int num)
{
    char buf[MAXBUF];

    _astr_itoa(num, buf, 16);

    _aio_puts(buf);
}

void _aio_putbin(int num)
{
    char buf[MAXBUF];

    _astr_itoa(num, buf, 2);

    _aio_puts(buf);
}

