
#include "atoi.h"

#define MAXBUF 40

void lowlevel_puts(char *str);

void putdec(int num)
{
    char buf[MAXBUF];

    if (num<0)
    {
        lowlevel_puts("-");
        num *= -1;
    }
    else
    {
        lowlevel_puts(" ");
    }

    itoa(num, buf, 10);

    lowlevel_puts(buf);
}

void puthex(int num)
{
    char buf[MAXBUF];

    itoa(num, buf, 16);

    lowlevel_puts(buf);
}

void putbin(int num)
{
    char buf[MAXBUF];

    itoa(num, buf, 2);

    lowlevel_puts(buf);
}

