#ifndef __amigaos__
#include "ui.h"
#include "tui.h"

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

static UI_event_cb     g_event_cb = NULL;
static void           *g_event_cb_user_data = NULL;
static uint16_t        g_scrollStart   = 0;
static uint16_t        g_scrollEnd     = 10;
static uint16_t        g_curLineStart  = 1;
static uint16_t        g_curLineCols   = 80;

uint16_t               UI_size_cols=80, UI_size_rows=25;

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
        case UI_TEXT_STYLE_DIALOG:
            UI_printf ( CSI "%dm", UI_STYLE_NORMAL);
            UI_printf ( CSI "%dm", UI_STYLE_INVERSE);
            UI_printf ( CSI "%dm", UI_STYLE_BLUE);
            break;
        default:
            assert(FALSE);
    }
}

void UI_beginLine (uint16_t row, uint16_t col_start, uint16_t cols)
{
    UI_moveCursor (row, col_start);
    g_curLineStart = col_start;
    g_curLineCols  = cols;
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

void UI_endLine (void)
{
    int16_t line_end = g_curLineCols+g_curLineStart-1;

    if ( line_end < UI_size_cols)
    {
        UI_flush();
        uint16_t r, c;
        getCursorPosition(&r, &c);
        for (uint16_t i = c; i<=line_end; i++)
            UI_putc(' ');
    }
    else
    {
        UI_putstr (CSI "K");    // erase to EOL
    }
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
    UI_flush();
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

static void updateTerminalSize(void)
{
    struct winsize ws;

    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
	{
        uint16_t orig_row, orig_col;

        if (!getCursorPosition(&orig_row, &orig_col))
        {
            assert(FALSE);
			goto failed;
        }

        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
        {
            assert(FALSE);
			goto failed;
        }
        if (!getCursorPosition(&UI_size_rows, &UI_size_cols))
        {
            assert(FALSE);
			goto failed;
        }

        char seq[32];
        snprintf(seq, 32, "\x1b[%d;%dH", orig_row, orig_col);
        write(STDOUT_FILENO, seq, strlen(seq));
    }
	else
	{
        UI_size_cols = ws.ws_col;
        UI_size_rows = ws.ws_row;
    }
    return;

failed:
    UI_size_cols = 80;
    UI_size_rows = 25;
}

void UI_onSizeChangeCall (UI_size_cb cb, void *user_data)
{
    g_size_cb = cb;
    g_size_cb_user_data = user_data;
}

static void handleSigWinCh(int unused __attribute__((unused)))
{
    updateTerminalSize();
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

    updateTerminalSize();

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
        if (g_event_cb)
            g_event_cb (ch, g_event_cb_user_data);
    }
}

uint16_t UI_EZRequest (char *body, char *gadgets, ...)
{
    va_list args;
    va_start(args, gadgets);
    static char buf2[1024];
    vsnprintf (buf2, 1024, body, args);
    va_end(args);
    return TUI_EZRequest (buf2, gadgets);
}

char *UI_FileReq  (char *title)
{
    // FIXME: implement
    assert(FALSE);
}

bool UI_FindReq (char *buf, uint16_t buf_len, bool *matchCase, bool *wholeWord, bool *searchBackwards)
{
    return TUI_FindReq (buf, buf_len, matchCase, wholeWord, searchBackwards);
}

void UI_HelpBrowser (void)
{
    // FIXME TUI_HelpBrowser();
}

void UI_onEventCall (UI_event_cb cb, void *user_data)
{
    g_event_cb           = cb;
    g_event_cb_user_data = user_data;
}

void UI_tprintf (char* format, ...)
{
    va_list args;
    va_start(args, format);
    UI_tvprintf (format, args);
    va_end(args);
}

void UI_tvprintf (char* format, va_list args)
{
    static char buf[BUFSIZE];
    int l = vsnprintf (buf, BUFSIZE, format, args);

    static uint16_t col = 0;
    static bool haveLine = FALSE;
    for (int i =0; i<l; i++)
    {
        if (!haveLine)
        {
            UI_scrollUp  (/*fullscreen=*/TRUE);
            UI_beginLine (UI_size_rows, 1, UI_size_cols);
            haveLine = TRUE;
        }
        if (col >= UI_size_cols)
        {
            UI_endLine ();
            haveLine = FALSE;
            col = 0;
        }
        char c = buf[i];
        if (c=='\n')
        {
            UI_endLine ();
            haveLine = FALSE;
            col = 0;
        }
        else
        {
            UI_putc(c);
            col++;
        }
    }
    if (haveLine)
        UI_endLine ();
    UI_moveCursor(UI_size_rows, col+1);
}


#endif
