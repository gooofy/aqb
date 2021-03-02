
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

/*********************************************************
 *
 * [LINE] INPUT support
 *
 *********************************************************/

void _aio_line_input (char *prompt, char **s, BOOL do_nl)
{
    if (prompt)
        _aio_puts(prompt);

    _aio_gets(s, do_nl);
}

#define MAX_TOKEN_LEN 1024

static char *g_input_buffer;
static int   g_input_pos;
static int   g_input_len;
static BOOL  g_input_eof;
static char  g_input_ch;
static char  g_input_token[MAX_TOKEN_LEN+1]; // add room for final \0

static void _input_getch (void)
{
    if (g_input_pos >= g_input_len)
    {
        g_input_eof = TRUE;
        return;
    }
    g_input_ch = g_input_buffer[g_input_pos++];
}

static BOOL _input_is_whitespace(void)
{
    return (g_input_ch == ' ') || (g_input_ch == '\t');
}

static void _input_skip_whitespace (void)
{
    while (_input_is_whitespace () && !g_input_eof)
        _input_getch ();
}

static void _input_skip_delimiter (void)
{
    _input_skip_whitespace();

    if (g_input_eof)
        return;

    BOOL delim_found = FALSE;
    while (!delim_found && !g_input_eof)
    {
        switch (g_input_ch)
        {
            case ',':
            case '\n':
                _input_getch();
                delim_found = TRUE;
                break;

            case '\r':
                _input_getch();
                if (g_input_ch == '\n')
                {
                    _input_getch();
                    delim_found = TRUE;
                }
                break;
            default:
                _input_getch();
                break;
        }
    }
}

static void _input_next_token (void)
{
    // _aio_puts("skip whitespace...\n");
    _input_skip_whitespace();
    if (g_input_eof)
        ERROR (AE_INPUT_OUT_OF_DATA); // FIXME: in console input, re-prompt the user

    BOOL skip_delim = TRUE;
    BOOL in_quote   = FALSE;
    int  len        = 0;

    while (!g_input_eof && (len < MAX_TOKEN_LEN))
    {
        // _aio_puts("token... \n");
        switch (g_input_ch)
        {
            case '\n':
                skip_delim = FALSE;
                goto fini;

            case '\r':
                _input_getch();
                if (g_input_ch == '\n')
                {
                    _input_getch();
                    skip_delim = FALSE;
                    goto fini;
                }
                break;

            case '"':
                if (in_quote)
                {
                    _input_getch();
                    goto fini;
                }

                if (len == 0)
                {
                    in_quote = TRUE;
                    _input_getch();
                    continue;
                }
                break;

            case ',':
                if (!in_quote)
                {
                    _input_getch();
                    skip_delim = FALSE;
                    goto fini;
                }
                break;

            case '\t':
            case ' ':
                if (!in_quote)
                {
                    _input_getch();
                    continue;
                }
                break;

            default:
                break;
        }

        g_input_token[len++] = g_input_ch;
        _input_getch();
    }

fini:
    g_input_token[len++] = '\0';

    // _aio_puts("delim\n");
    if (skip_delim)
        _input_skip_delimiter ();
}

void _aio_console_input (BOOL qm, char *prompt, BOOL do_nl)
{
    char *p = qm ? prompt : "?";
    _aio_line_input (p, &g_input_buffer, do_nl);
    g_input_pos = 0;
    g_input_len = LEN_(g_input_buffer);
    g_input_eof = FALSE;
    _input_getch();
}

void _aio_inputs1 (BYTE   *v)
{
    _input_next_token ();
    *v = VALINT_ (g_input_token);
}
void _aio_inputu1 (UBYTE  *v)
{
    _input_next_token ();
    *v = VALINT_ (g_input_token);
}
void _aio_inputs2 (SHORT  *v)
{
    _input_next_token ();
    *v = VALINT_ (g_input_token);
}
void _aio_inputu2 (USHORT *v)
{
    _input_next_token ();
    *v = VALUINT_ (g_input_token);
}
void _aio_inputs4 (LONG   *v)
{
    _input_next_token ();
    *v = VALLNG_ (g_input_token);
}
void _aio_inputu4 (ULONG  *v)
{
    _input_next_token ();
    *v = VALULNG_ (g_input_token);
}
void _aio_inputf  (FLOAT  *v)
{
    _input_next_token ();
    *v = VAL_ (g_input_token);
}
void _aio_inputs (char  **v)
{
    _input_next_token ();
    *v = _astr_dup(g_input_token);
}


