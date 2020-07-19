
#include "../_brt/_brt.h"
#include "_aqb.h"

#include <clib/dos_protos.h>
#include <inline/dos.h>

#define MAXBUF 40

void _aio_init(void)
{
}

void _aio_shutdown(void)
{
}

void _aio_putnl(void)
{
    _aio_puts("\n");
}

void _aio_puts4(int num)
{
    char buf[MAXBUF];

    _astr_itoa(num, buf, 10);

    _aio_puts(buf);
}

void _aio_puts2(short num)
{
    char buf[MAXBUF];

    _astr_itoa(num, buf, 10);

    _aio_puts(buf);
}

void _aio_puts1(char num)
{
    char buf[MAXBUF];

    _astr_itoa(num, buf, 10);

    _aio_puts(buf);
}

void _aio_putu4(unsigned int num)
{
    char buf[MAXBUF];

    _astr_utoa(num, buf, 10);

    _aio_puts(buf);
}

void _aio_putu2(unsigned short num)
{
    char buf[MAXBUF];

    _astr_utoa(num, buf, 10);

    _aio_puts(buf);
}

void _aio_putu1(unsigned char num)
{
    char buf[MAXBUF];

    _astr_utoa(num, buf, 10);

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

void _aio_putuhex(ULONG l)
{
    char  buf[MAXBUF];
    ULONG digit;

    for (int i = 7; i>=0; i--)
    {
        digit = (l >> (4*i)) & 0xf;

        if (digit < 10)
            buf[7-i] = '0' + digit;
        else
            buf[7-i] = 'a' + (digit-10);
    }
    buf[8] = 0;

    _aio_puts(buf);
}

void _aio_putf(FLOAT f)
{
    char buf[40];
    _astr_ftoa(f, buf);
    _aio_puts(buf);
}

void _aio_putbool(BOOL b)
{
    _aio_puts(b ? "TRUE" : "FALSE");
}

