
#include "astring.h"

#define MAXBUF 40

void lowlevel_puts(char *str);

void puts4(int num)
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

void puts2(short num)
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

#if 0
extern short bar(void);

void foo(void)
{
    short s, t;

    s = bar();
    t = bar();

    puts2(s/t);
}
#endif

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

