
#include "astring.h"
#include "io.h"

#define MAXBUF 40

void puts4(int num)
{
    char buf[MAXBUF];

    if (num<0)
    {
        aputs("-");
        num *= -1;
    }
    else
    {
        aputs(" ");
    }

    itoa(num, buf, 10);

    aputs(buf);
}

void puts2(short num)
{
    char buf[MAXBUF];

    if (num<0)
    {
        aputs("-");
        num *= -1;
    }
    else
    {
        aputs(" ");
    }

    itoa(num, buf, 10);

    aputs(buf);
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

    aputs(buf);
}

void putbin(int num)
{
    char buf[MAXBUF];

    itoa(num, buf, 2);

    aputs(buf);
}

