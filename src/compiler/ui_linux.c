#ifndef __amigaos__
#include "ui.h"

#include <stdlib.h>
#include <stdarg.h>
#include "logger.h"

#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#define CSI       "\033["

#define UI_STYLE_NORMAL     0
#define UI_STYLE_BOLD       1
#define UI_STYLE_ITALICS    3
#define UI_STYLE_UNDERLINE  4
#define UI_STYLE_INVERSE    7

#define UI_STYLE_BLACK     30
#define UI_STYLE_RED       31
#define UI_STYLE_GREEN     32
#define UI_STYLE_YELLOW    33
#define UI_STYLE_BLUE      34
#define UI_STYLE_MAGENTA   35
#define UI_STYLE_CYAN      36
#define UI_STYLE_WHITE     37

#define BUFSIZE   2048
static char            g_outbuf[BUFSIZE];
static int             g_bpos = 0;
static UI_size_cb      g_size_cb = NULL;
static void           *g_size_cb_user_data = NULL;

static UI_key_cb       g_key_cb = NULL;
static void           *g_key_cb_user_data = NULL;
static uint16_t        g_scrollStart   = 0;
static uint16_t        g_scrollEnd     = 10;

static void UI_flush  (void)
{
    if (g_bpos != 0)
        write(STDOUT_FILENO, g_outbuf, g_bpos);
    g_bpos = 0;
}

void UI_setTextStyle (uint16_t style)
{
    switch (style)
    {
        case UI_TEXT_STYLE_TEXT:
            UI_printf ( CSI "%dm", UI_STYLE_NORMAL);
            break;
        case UI_TEXT_STYLE_KEYWORD:
            UI_printf ( CSI "%dm", UI_STYLE_BOLD);
            UI_printf ( CSI "%dm", UI_STYLE_YELLOW);
            break;
        case UI_TEXT_STYLE_COMMENT:
            UI_printf ( CSI "%dm", UI_STYLE_BOLD);
            UI_printf ( CSI "%dm", UI_STYLE_BLUE);
            break;
        case UI_TEXT_STYLE_INVERSE:
            UI_printf ( CSI "%dm", UI_STYLE_NORMAL);
            UI_printf ( CSI "%dm", UI_STYLE_INVERSE);
            break;
        default:
            assert(FALSE);
    }
}

void UI_beginLine (uint16_t row)
{
    UI_moveCursor (row, 1);
}

void UI_putc(char c)
{
    g_outbuf[g_bpos++] = c;
    if (g_bpos >= BUFSIZE)
        UI_flush();
}

void UI_putstr(char *s)
{
    while (*s)
    {
        g_outbuf[g_bpos++] = *s++;
        if (g_bpos >= BUFSIZE)
            UI_flush();
    }
}

void UI_printf (char* format, ...)
{
    va_list args;
    va_start(args, format);
    UI_vprintf (format, args);
    va_end(args);
}

void UI_vprintf (char* format, va_list args)
{
    static char buf[BUFSIZE];
    vsnprintf (buf, BUFSIZE, format, args);
    UI_putstr(buf);
}

void UI_endLine (void)
{
    UI_putstr (CSI "K");    // erase to EOL
    UI_flush();
}

void UI_setCursorVisible (bool visible)
{
    if (visible)
        UI_putstr ( CSI "?25h");
    else
        UI_putstr ( CSI "?25l");
    UI_flush();
}

void UI_moveCursor (uint16_t row, uint16_t col)
{
    UI_printf (CSI "%d;%d;H", row, col);
}

static uint16_t UI_getch (void)
{
    int nread;
    char c, seq[5];
    while ((nread = read(STDIN_FILENO,&c,1)) == 0);
    if (nread == -1)
        exit(1);

    while(1)
    {
        if (c==KEY_ESC) // handle escape sequences
        {
            LOG_printf (LOG_DEBUG, "terminal: UI_getch(): handling escape sequence...\n");
            /* If this is just an ESC, we'll timeout here. */
            if (read(STDIN_FILENO,seq,1) == 0)
            {
                LOG_printf (LOG_DEBUG, "terminal: UI_getch(): ESC timeout 1\n");
                return KEY_ESC;
            }
            if (read(STDIN_FILENO,seq+1,1) == 0)
            {
                LOG_printf (LOG_DEBUG, "terminal: UI_getch(): ESC timeout 2\n");
                return KEY_UNKNOWN1;
            }

            switch (seq[0])
            {
                case '[':       // ESC [ ...

                    switch (seq[1])
                    {
                        case '1':       // ESC [ 1 ...
                            if (read(STDIN_FILENO,seq+2,2) == 0)
                            {
                                LOG_printf (LOG_DEBUG, "terminal: UI_getch(): ESC timeout 3\n");
                                return KEY_UNKNOWN1;
                            }
                            LOG_printf (LOG_DEBUG, "terminal: UI_getch(): escape sequence detected: ESC %c [0x%02x] %c [0x%02x] %c [0x%02x] %c [0x%02x]\n", seq[0], seq[0], seq[1], seq[1], seq[2], seq[2], seq[3], seq[3]);
                            switch (seq[2])
                            {
                                case '5': return KEY_F5;
                                case '7': return KEY_F6;
                                case '8': return KEY_F7;
                                case '9': return KEY_F8;
                                case '~': return KEY_HOME;
                                case ';':
                                    if (read(STDIN_FILENO,seq+4,1) == 0)
                                    {
                                        LOG_printf (LOG_DEBUG, "terminal: UI_getch(): ESC timeout 4\n");
                                        return KEY_UNKNOWN1;
                                    }
                                    switch (seq[4])
                                    {
                                        case 'A':
                                            return KEY_PAGE_UP;   // SHIFT + CURSOR_UP
                                        case 'B':
                                            return KEY_PAGE_DOWN; // SHIFT + CURSOR_DOWN
                                        default:
                                            LOG_printf (LOG_DEBUG, "terminal: UI_getch(): unknown escape sequence detected: ESC %c [0x%02x] %c [0x%02x] %c [0x%02x] %c [0x%02x] %c [0x%02x]\n", seq[0], seq[0], seq[1], seq[1], seq[2], seq[2], seq[3], seq[3], seq[4], seq[4]);
                                    }
                                    return KEY_UNKNOWN2;
                            }
                            break;
                        case '2':       // ESC [ 2 ...
                            if (read(STDIN_FILENO,seq+2,1) == 0)
                            {
                                LOG_printf (LOG_DEBUG, "terminal: UI_getch(): ESC timeout 4\n");
                                return KEY_ESC;
                            }
                            LOG_printf (LOG_DEBUG, "terminal: UI_getch(): escape sequence detected: ESC %c [0x%02x] %c [0x%02x] %c [0x%02x]\n", seq[0], seq[0], seq[1], seq[1], seq[2], seq[2]);
                            switch (seq[2])
                            {
                                case '0': return KEY_F9;
                                case '1': return KEY_F10;
                                default: return KEY_UNKNOWN1;
                            }
                            break;
                        case '3':       // ESC [ 3 ...
                            if (read(STDIN_FILENO,seq+2,1) == 0)
                            {
                                LOG_printf (LOG_DEBUG, "terminal: UI_getch(): ESC timeout 4\n");
                                return KEY_ESC;
                            }
                            LOG_printf (LOG_DEBUG, "terminal: UI_getch(): escape sequence detected: ESC %c [0x%02x] %c [0x%02x] %c [0x%02x]\n", seq[0], seq[0], seq[1], seq[1], seq[2], seq[2]);
                            switch (seq[2])
                            {
                                case '~': return KEY_DEL;
                                default: return KEY_UNKNOWN1;
                            }
                            break;
                        case '4':       // ESC [ 4 ...
                            if (read(STDIN_FILENO,seq+2,1) == 0)
                            {
                                LOG_printf (LOG_DEBUG, "terminal: UI_getch(): ESC timeout 4\n");
                                return KEY_ESC;
                            }
                            LOG_printf (LOG_DEBUG, "terminal: UI_getch(): escape sequence detected: ESC %c [0x%02x] %c [0x%02x] %c [0x%02x]\n", seq[0], seq[0], seq[1], seq[1], seq[2], seq[2]);
                            switch (seq[2])
                            {
                                case '~': return KEY_END;
                                default: return KEY_UNKNOWN2;
                            }
                            break;
                        case '5':       // ESC [ 5 ...
                            if (read(STDIN_FILENO,seq+2,1) == 0)
                            {
                                LOG_printf (LOG_DEBUG, "terminal: UI_getch(): ESC timeout 4\n");
                                return KEY_ESC;
                            }
                            LOG_printf (LOG_DEBUG, "terminal: UI_getch(): escape sequence detected: ESC %c [0x%02x] %c [0x%02x] %c [0x%02x]\n", seq[0], seq[0], seq[1], seq[1], seq[2], seq[2]);
                            switch (seq[2])
                            {
                                case '~': return KEY_PAGE_UP;
                                default: return KEY_UNKNOWN3;
                            }
                            break;
                        case '6':       // ESC [ 6 ...
                            if (read(STDIN_FILENO,seq+2,1) == 0)
                            {
                                LOG_printf (LOG_DEBUG, "terminal: UI_getch(): ESC timeout 4\n");
                                return KEY_ESC;
                            }
                            LOG_printf (LOG_DEBUG, "terminal: UI_getch(): escape sequence detected: ESC %c [0x%02x] %c [0x%02x] %c [0x%02x]\n", seq[0], seq[0], seq[1], seq[1], seq[2], seq[2]);
                            switch (seq[2])
                            {
                                case '~': return KEY_PAGE_DOWN;
                                default: return KEY_UNKNOWN4;
                            }
                            break;
                        case 'A': return KEY_CURSOR_UP;
                        case 'B': return KEY_CURSOR_DOWN;
                        case 'C': return KEY_CURSOR_RIGHT;
                        case 'D': return KEY_CURSOR_LEFT;
                        default:
                            LOG_printf (LOG_DEBUG, "terminal: UI_getch(): escape sequence detected: ESC %c [0x%02x] %c [0x%02x] *** UNKNOWN ***\n", seq[0], seq[0], seq[1], seq[1]);
                            return KEY_UNKNOWN5;
                    }
                    break;
                case 'O':       // ESC O ...
                    LOG_printf (LOG_DEBUG, "terminal: UI_getch(): escape sequence detected: ESC %c [0x%02x] %c [0x%02x]\n", seq[0], seq[0], seq[1], seq[1]);
                    switch(seq[1])
                    {
                        case 'H': return KEY_HOME;
                        case 'F': return KEY_END;
                        case 'P': return KEY_F1;
                        case 'Q': return KEY_F2;
                        case 'R': return KEY_F3;
                        case 'S': return KEY_F4;
                        default: return KEY_UNKNOWN6;
                    }
                    break;
                default:
                    LOG_printf (LOG_DEBUG, "terminal: UI_getch(): escape sequence detected: ESC %c [0x%02x] %c [0x%02x] *** UNKNOWN ***\n", seq[0], seq[0], seq[1], seq[1]);
                    return KEY_UNKNOWN6;

            }
        }
        else
        {
            if (c==127)
                return KEY_BACKSPACE;
            return c;
        }
    }
    return 0;
}

uint16_t UI_waitkey (void)
{
    return UI_getch();
}

void UI_bell (void)
{
    UI_putstr ("\007");
    UI_flush();
}

void UI_eraseDisplay (void)
{
    UI_moveCursor (1, 1);
    UI_putstr (CSI "J");
}

void UI_setColorScheme (int scheme)
{
    // FIXME: not supported
}

void UI_setFont (int font)
{
    // FIXME: not supported
}

void UI_setScrollArea (uint16_t row_start, uint16_t row_end)
{
    g_scrollStart = row_start;
    g_scrollEnd   = row_end;
    UI_printf (CSI "%d;%dt", row_start, row_end);
    UI_flush();
}

void UI_scrollUp (bool fullscreen)
{
    if (fullscreen)
        UI_printf (CSI "t");

    UI_printf ( CSI "S");

    if (fullscreen)
        UI_printf (CSI "%d;%dt", g_scrollStart, g_scrollEnd);
    UI_flush();
}

void UI_scrollDown (void)
{
    UI_printf ( CSI "T");
    UI_flush();
}

static bool getCursorPosition(uint16_t *rows, uint16_t *cols)
{
    char buf[32];
    unsigned int i = 0;

    UI_flush();

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
		return FALSE;

    while (i < sizeof(buf)-1)
	{
        if (read(STDIN_FILENO,buf+i,1) != 1)
			break;
        if (buf[i] == 'R')
			break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != KEY_ESC || buf[1] != '[')
		return FALSE;
    if (sscanf(buf+2,"%hd;%hd",rows,cols) != 2)
		return FALSE;
    return TRUE;
}

bool UI_getsize(uint16_t *rows, uint16_t *cols)
{
    struct winsize ws;

    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
	{
        uint16_t orig_row, orig_col;

        if (!getCursorPosition(&orig_row, &orig_col))
			goto failed;

        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
			goto failed;
        if (!getCursorPosition(rows, cols))
			goto failed;


        char seq[32];
        snprintf(seq, 32, "\x1b[%d;%dH", orig_row, orig_col);
        write(STDOUT_FILENO, seq, strlen(seq));
    }
	else
	{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
    }
	return TRUE;

failed:
    return FALSE;
}

void UI_onSizeChangeCall (UI_size_cb cb, void *user_data)
{
    g_size_cb = cb;
    g_size_cb_user_data = user_data;
}

static void handleSigWinCh(int unused __attribute__((unused)))
{
    if (g_size_cb)
        g_size_cb(g_size_cb_user_data);
}

static void UI_setAlternateScreen (bool enabled)
{
    if (enabled)
        UI_printf (CSI "?1049h");
    else
        UI_printf (CSI "?1049l");
}

static struct termios g_orig_termios;
void UI_deinit (void)
{
    UI_setTextStyle (UI_STYLE_NORMAL);
    UI_moveCursor(0, 0);
    UI_eraseDisplay();
    UI_setAlternateScreen(FALSE);
    UI_flush();
	// disable raw mode
	tcsetattr(STDOUT_FILENO, TCSAFLUSH, &g_orig_termios);
}

bool UI_init (void)
{
    signal(SIGWINCH, handleSigWinCh);

	// enable raw mode
    struct termios raw;

    if (!isatty(STDIN_FILENO))
		goto fatal;
    if (tcgetattr(STDOUT_FILENO, &g_orig_termios) == -1)
		goto fatal;

    raw = g_orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDOUT_FILENO, TCSAFLUSH, &raw) < 0)
		goto fatal;
    return TRUE;

fatal:
    errno = ENOTTY;
    return FALSE;
}


void UI_run(void)
{
    bool running = TRUE;
    while (running)
    {
        uint16_t ch = UI_getch();
        LOG_printf (LOG_DEBUG, "terminal: UI_getch() returned %d\n", ch);
        if (g_key_cb)
            g_key_cb (ch, g_key_cb_user_data);
    }
}

uint16_t UI_EZRequest (char *body, char *gadgets)
{
    uint16_t rows, cols;

    UI_getsize (&rows, &cols);

    UI_scrollUp(/*fullscreen=*/TRUE);
    UI_scrollUp(/*fullscreen=*/TRUE);

    UI_moveCursor (rows, 1);

    UI_setTextStyle (UI_STYLE_NORMAL);

    for (char *p = body; *p; p++)
    {
        if (*p != '\n')
        {
            UI_putc(*p);
        }
        else
        {
            UI_scrollUp(/*fullscreen=*/TRUE);
            UI_moveCursor (rows, 1);
        }
    }
    UI_scrollUp(/*fullscreen=*/TRUE);
    UI_moveCursor (rows, 1);

    uint16_t cnt;
    char *c = gadgets;
    static char buf[256];
    char *s = buf;
    while (*c)
    {
        if (*c=='|')
        {
            cnt++;
            *s = 0;
            UI_printf ("%s", buf);
            UI_printf ("[%d] ", cnt);
            c++;
            s = buf;
        }
        else
        {
            *s++ = *c++;
        }
    }
    *s = 0;
    UI_printf ("%s", buf);
    UI_printf ("[0]", cnt);
    cnt++;

    UI_flush();

    uint16_t res = 0;
    bool running = TRUE;
    while (running)
    {
        uint16_t ch = UI_getch();

        switch(ch)
        {
			case 13:
                res = 1;
                running = FALSE;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                res = ch - '0';
                running = FALSE;
                break;
        }
    }

    UI_eraseDisplay ();
    return res;
}

char *UI_FileReq  (char *title)
{
    // FIXME: implement
    assert(FALSE);
}

bool UI_lineInput (uint16_t row, char *prompt, char *buf, uint16_t buf_len)
{
    // FIXME: implement
    assert(FALSE);
    return FALSE;
}

void UI_onKeyCall (UI_key_cb cb, void *user_data)
{
    g_key_cb           = cb;
    g_key_cb_user_data = user_data;
}


#endif
