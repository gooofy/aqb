// IDE console view implementation

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "ide.h"
#include "ui.h"
#include "logger.h"
#include "util.h"

static void _repaintConsole (IDE_instance ed)
{
    UI_view view = ed->view_console;
    UI_clearView(view);

    /*
     * MAX_CON_LINES = 128
     * ed->con_rows  = 5
     *
     * offset =
     * MAX_CON_LINES-ed->con_rows =
     *            123                        118
     *
     *            console view   buffer      i console view   buffer
     *            line  txt                    line txt
     *            1     l4       [38]        0 1              [33]
     *            2     l3       [39]        1 2              [34]
     *            3     l2       [40]        2 3              [35]
     *            4     l1       [41]        3 4              [36]
     * con_line-> 5     l0       [42]        4 5              [ con_line - MAX_CON_LINES + offset + i + 1]
     *                                                        42       - 128           + 123      + 4   = 41
     *
     */

    LOG_printf (LOG_DEBUG, "ide_con: _repaint_console: ed->con_rows=%d\n", ed->con_rows);

    int16_t offset = UI_getViewScrollPos (view);

    ed->con_buf[ed->con_line][ed->con_col]=0;
    uint8_t s = UI_TEXT_STYLE_TEXT;
    UI_setTextStyle (view, s);

    for (int16_t i=0; i<ed->con_rows; i++)
    {
        int16_t l = ed->con_line - MAX_CON_LINES + offset + 1 + i;

        int16_t l2 = l>=0 ? l : MAX_CON_LINES + l;

        LOG_printf (LOG_DEBUG, "ide_con: _repaint_console: l = ed->con_line=%d - MAX_CON_LINES=%d + offset=%d + 1 + i=%d -> %d (%d) %s\n",
                    ed->con_line, MAX_CON_LINES, offset, i, l, l2, ed->con_buf[l2]);

        UI_beginLine (ed->view_console, i+1, 1, ed->con_cols);

        int16_t bufl = strlen(ed->con_buf[l2]);
        for (int16_t j=0; j<bufl; j++)
        {
            uint8_t s2 = ed->con_style[l2][j];
            if (s2 != s)
            {
                s = s2;
                UI_setTextStyle (view, s);
            }
            UI_putc (ed->view_console, ed->con_buf[l2][j]);
        }

        UI_endLine (ed->view_console);
    }
}

static void _console_size_cb (IDE_instance ed)
{
    UI_view view = ed->view_console;

    LOG_printf (LOG_DEBUG, "IDE: _console_size_cb called.\n");

    // FIXME
    UI_getViewSize (view, &ed->con_rows, &ed->con_cols);
    UI_cfgViewScroller (ed->view_console, MAX_CON_LINES-ed->con_rows, MAX_CON_LINES, ed->con_rows);
    _repaintConsole (ed);
}

static void _console_event_cb (UI_view view, uint16_t key, void *user_data)
{
	IDE_instance ed = (IDE_instance) user_data;

    switch (key)
    {
        case KEY_STOPPED:
            LOG_printf (LOG_INFO, "\n\n*** PROGRAM EXITED ***\n\n");
            IDE_conSet (ed, /*visible=*/TRUE, /*active=*/FALSE);
            UI_updateMenu (/*inDebugMode=*/FALSE);
            break;
        case KEY_RESIZE:
            _console_size_cb (ed);
            break;
        case KEY_SCROLLV:
            _repaintConsole (ed);
            break;
        default:
            UI_bell();
    }
}

void IDE_conInit (IDE_instance ed)
{
    ed->con_line = 0;
    ed->con_col  = 0;
    ed->view_console = UI_getView (UI_viewConsole);
    for (int16_t i=0; i<MAX_CON_LINES; i++)
        ed->con_buf[i][0]=0;

    UI_onEventCall (ed->view_console, _console_event_cb, ed);
    _console_size_cb (ed);
    UI_setCursorVisible (ed->view_console, FALSE);
}

void IDE_conSet (IDE_instance ed, bool visible, bool active)
{
    if (active)
        UI_activateView (ed->view_console);
    else
        UI_activateView (ed->view_editor);
}

void IDE_cprintf (IDE_instance ed, char* format, ...)
{
    va_list args;
    va_start(args, format);
    IDE_cvprintf (ed, format, args);
    va_end(args);
}

static void _scrollToBottom (IDE_instance ed)
{
    UI_view view = ed->view_console;

    // scroll to bottom

    int16_t offset = UI_getViewScrollPos (view);
    if (offset != MAX_CON_LINES-ed->con_rows)
        _console_size_cb (ed);
}

#define CSI_BUF_LEN 16

// provides simplistic ANSI terminal emulation

void IDE_cvprintf (IDE_instance ed, char* format, va_list args)
{
    if (!ed)
        return;

    UI_view view = ed->view_console;

    static char buf[UI_MAX_COLUMNS];
    int l = vsnprintf (buf, UI_MAX_COLUMNS, format, args);

    if (!UI_isViewVisible (view))
        UI_setViewVisible (view, TRUE);

    _scrollToBottom (ed);

    if (ed->con_col >= ed->con_cols)
    {
        UI_scrollUp  (view);
        ed->con_buf[ed->con_line][ed->con_col]=0;
        ed->con_line = (ed->con_line+1) % MAX_CON_LINES;
        ed->con_col=0;
    }

    uint16_t cols = ed->con_cols-ed->con_col;

    LOG_printf (LOG_DEBUG, "IDE: IDE_cvprintf buf=%s, ed->con_rows=%d, ed->con_cols=%d, ed->con_col=%d -> cols=%d\n",
                           buf, ed->con_rows, ed->con_cols, ed->con_col, cols);

    UI_beginLine (view, ed->con_rows, ed->con_col+1, cols);

    bool     bCSI = FALSE;
    char     csiBuf[CSI_BUF_LEN];
    uint16_t csiBufLen=0;

    char s = UI_getTextStyle (view);

    for (int i =0; i<l; i++)
    {
        char c = buf[i];
        if ((c=='\n') || (ed->con_col>=MAX_CON_LINE_LEN-1))
        {
            UI_endLine (view);
            UI_scrollUp  (view);
            ed->con_buf[ed->con_line][ed->con_col]=0;
            ed->con_line = (ed->con_line+1) % MAX_CON_LINES;
            ed->con_col=0;
            UI_beginLine (view, ed->con_rows, 1, ed->con_cols-ed->con_col);
        }
        else
        {

            uint8_t uc = (uint8_t) c;
            if (!bCSI)
            {
                if (uc==0x9b)
                {
                    bCSI = TRUE;
                    continue;
                }
            }
            else
            {
                char s2 = s;
                /*
                0x30–0x3F (ASCII 0–9:;<=>?)                  parameter bytes
                0x20–0x2F (ASCII space and !\"#$%&'()*+,-./) intermediate bytes
                0x40–0x7E (ASCII @A–Z[\]^_`a–z{|}~)          final byte
                */
                if (uc>=0x40)
                {
                    //LOG_printf (LOG_DEBUG, "!CSI seq detected: %s%c\n", csiBuf, c);
                    //printf ("CSI seq detected: %s%c csiBufLen=%d\n", csiBuf, c, csiBufLen);

                    switch (c)
                    {
                        case 'p': // csr on/off
                            if (csiBufLen == 2)
                            {
                                switch (csiBuf[0])
                                {
                                    case '0':
                                       UI_setCursorVisible (view, FALSE);
                                       break;
                                    case '1':
                                       UI_setCursorVisible (view, TRUE);
                                       break;
                                }
                            }
                            break;
                        case 'm': // presentation
                            if (csiBufLen == 1)
                            {
                                switch (csiBuf[0])
                                {
                                    case '0': s2 = UI_TEXT_STYLE_TEXT; break;
                                }
                            }
                            else
                            {
                                if (csiBufLen == 2)
                                {
                                    uint8_t color = (csiBuf[0]-'0')*10+(csiBuf[1]-'0');
                                    //printf ("setting color %d\n", color);
                                    switch (color)
                                    {
                                        case 30: s2 = UI_TEXT_STYLE_ANSI_0; break;
                                        case 31: s2 = UI_TEXT_STYLE_ANSI_1; break;
                                        case 32: s2 = UI_TEXT_STYLE_ANSI_2; break;
                                        case 33: s2 = UI_TEXT_STYLE_ANSI_3; break;
                                        case 34: s2 = UI_TEXT_STYLE_ANSI_4; break;
                                        case 35: s2 = UI_TEXT_STYLE_ANSI_5; break;
                                        case 36: s2 = UI_TEXT_STYLE_ANSI_6; break;
                                        case 37: s2 = UI_TEXT_STYLE_ANSI_7; break;
                                    }
                                }
                            }
                            break;
                    }

                    bCSI = FALSE;
                    csiBufLen = 0;
                }
                else
                {
                    if (csiBufLen<CSI_BUF_LEN)
                        csiBuf[csiBufLen++] = c;
                }
                if (s2 != s)
                {
                    s = s2;
                    UI_setTextStyle (view, s);
                }
                continue;
            }

            ed->con_buf  [ed->con_line][ed->con_col] = c;
            ed->con_style[ed->con_line][ed->con_col] = s;
            UI_putc(view, c);
            ed->con_col++;
        }
    }
    UI_endLine (view);
    UI_moveCursor(view, ed->con_rows, ed->con_col+1);
}

static void _readline_repaint(IDE_instance ed, char *buf, int16_t cursor_pos, int16_t *scroll_offset, int16_t row, int16_t col, int16_t width)
{
    UI_view view = ed->view_console;
    _scrollToBottom (ed);
    UI_setTextStyle (view, UI_TEXT_STYLE_TEXT);

    int16_t cp=0;
    while (TRUE)
    {
        cp = cursor_pos - *scroll_offset;
        if ( (cp>=0) && (cp<width) )
            break;
        else if (cp<0)
            *scroll_offset -= 1;
        else
            *scroll_offset += 1;
    }

    //LOG_printf (LOG_DEBUG, "_readline_repaint: UI_beginLine col=%d, width=%d, ed->con_line=%d\n", col, width, ed->con_line);
    //LOG_printf (LOG_DEBUG, "_readline_repaint: con_buf[%d]=%s\n", ed->con_line, ed->con_buf[ed->con_line]);
    UI_beginLine (view, row, col, width);
    int16_t l = strlen(buf);
    for (uint16_t c = 0; c<width; c++)
    {
        int16_t bp = c+*scroll_offset;
        if (bp<l)
        {
            UI_putc(view, buf[bp]);
            ed->con_buf  [ed->con_line][col+c-1] = buf[bp];
            ed->con_col = col+c;
        }
        else
        {
            UI_putc (view, ' ');
            ed->con_buf  [ed->con_line][col+c-1] = 0;
        }
        ed->con_style[ed->con_line][col+c-1] = UI_TEXT_STYLE_TEXT;
    }
    UI_endLine (view);
    UI_moveCursor (view, row, col+cp);
}

uint16_t IDE_readline (IDE_instance ed, char *buf, int16_t buf_len)
{
    uint16_t res = KEY_NONE;

    UI_view view = ed->view_console;

    if (!UI_isViewVisible (view))
        UI_setViewVisible (view, TRUE);

    int16_t view_cols, view_rows;
    UI_getViewSize (view, &view_rows, &view_cols);

    _scrollToBottom (ed);

    int16_t scroll_offset=0;
    int16_t cursor_pos=0;

    uint16_t col, row;
    UI_getCursorPos (view, &row, &col);
    UI_setCursorVisible (view, TRUE);

    uint16_t width = view_cols-col-1;

    //LOG_printf (LOG_DEBUG, "IDE_readline: view_cols=%d, col=%d, width=%d\n", view_cols, col, width);

    bool finished = FALSE;

    while (!finished)
    {
        uint16_t event = UI_waitkey();
        switch (event)
        {
            case KEY_CURSOR_LEFT:
                if (cursor_pos>0)
                {
                    cursor_pos--;
                    _readline_repaint(ed, buf, cursor_pos, &scroll_offset, row, col, width);
                }
                else
                {
                    UI_bell();
                }
                break;

            case KEY_CURSOR_RIGHT:
                if (cursor_pos<(strlen(buf)))
                {
                    cursor_pos++;
                    _readline_repaint(ed, buf, cursor_pos, &scroll_offset, row, col, width);
                }
                else
                {
                    UI_bell();
                }
                break;

            case KEY_HOME:
                cursor_pos = 0;
                _readline_repaint(ed, buf, cursor_pos, &scroll_offset, row, col, width);
                break;

            case KEY_END:
                cursor_pos = strlen(buf);
                _readline_repaint(ed, buf, cursor_pos, &scroll_offset, row, col, width);
                break;

            case KEY_BACKSPACE:
                if (cursor_pos>0)
                {
                    uint16_t l = strlen(buf);
                    for (uint16_t i=cursor_pos; i<l; i++)
                        buf[i-1] = buf[i];
                    buf[l-1] = 0;
                    cursor_pos--;
                    _readline_repaint(ed, buf, cursor_pos, &scroll_offset, row, col, width);
                }
                else
                {
                    UI_bell();
                }
                break;

            case KEY_DEL:
            {
                uint16_t l = strlen(buf);
                if (cursor_pos<l)
                {
                    for (uint16_t i=cursor_pos; i<l-1; i++)
                        buf[i] = buf[i+1];
                    buf[l-1] = 0;
                    _readline_repaint(ed, buf, cursor_pos, &scroll_offset, row, col, width);
                }
                else
                {
                    UI_bell();
                }
                break;
            }

            case KEY_NONE:
                break;

            case KEY_ENTER:
                finished = TRUE;
                break;

            case KEY_F1:
            case KEY_F2:
            case KEY_F3:
            case KEY_F4:
            case KEY_F5:
            case KEY_F6:
            case KEY_F7:
            case KEY_F8:
            case KEY_F9:
            case KEY_F10:
                res = event;
                finished = TRUE;
                break;

            default:
            {
                uint16_t l = strlen(buf);
                if ( (event >= 32) && (event <= 126) && (l < buf_len-1) )
                {
                    for (int i=l; i>cursor_pos; i--)
                    {
                        buf[i]   = buf[i-1];
                    }
                    buf[cursor_pos] = event;
                    buf[l+1] = 0;
                    cursor_pos++;
                    _readline_repaint(ed, buf, cursor_pos, &scroll_offset, row, col, width);
                }
                else
                {
                    UI_bell();
                }
                break;
            }
        }
    }
    UI_setCursorVisible (view, FALSE);
    return res;
}

