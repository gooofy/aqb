// IDE console view implementation

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "ide.h"
#include "ui.h"
#include "logger.h"
#include "util.h"

static void _console_size_cb (UI_view view, void *user_data)
{
	IDE_instance ed = (IDE_instance) user_data;

    LOG_printf (LOG_DEBUG, "IDE: _console_size_cb called.\n");

    // FIXME
    UI_getViewSize (view, &ed->con_rows, &ed->con_cols);
    //_invalidateAll (ed);
    //_scroll(ed);
    UI_clearView(view);
    //_repaint(ed);
}

static void _console_event_cb (UI_view view, uint16_t key, void *user_data)
{
	IDE_instance ed = (IDE_instance) user_data;

    switch (key)
    {
        case KEY_STOPPED:
            LOG_printf (LOG_INFO, "\n\n*** PROGRAM EXITED ***\n\n");
            IDE_conSet (ed, /*visible=*/TRUE, /*active=*/FALSE);
            break;
        default:
            UI_bell();
    }
}

void IDE_conInit (IDE_instance ed)
{
    ed->view_console = UI_getView (UI_viewConsole);
    UI_onSizeChangeCall (ed->view_console, _console_size_cb, ed);
    UI_onEventCall (ed->view_console, _console_event_cb, ed);
    _console_size_cb (ed->view_console, ed);
    UI_setCursorVisible (ed->view_console, TRUE);
    ed->con_lines = 0;
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

void IDE_cvprintf (IDE_instance ed, char* format, va_list args)
{
    static char buf[UI_MAX_COLUMNS];
    int l = vsnprintf (buf, UI_MAX_COLUMNS, format, args);

    if (!UI_isViewVisible (ed->view_console))
        UI_setViewVisible (ed->view_console, TRUE);

    static uint16_t col = 0;
    static bool haveLine = FALSE;
    for (int i =0; i<l; i++)
    {
        if (!haveLine)
        {
            UI_scrollUp  (ed->view_console);
            UI_beginLine (ed->view_console, ed->con_rows, 1, ed->con_cols);
            haveLine = TRUE;
        }
        if (col >= ed->con_cols)
        {
            UI_endLine (ed->view_console);
            haveLine = FALSE;
            col = 0;
        }
        char c = buf[i];
        if (c=='\n')
        {
            UI_endLine (ed->view_console);
            haveLine = FALSE;
            col = 0;
        }
        else
        {
            UI_putc(ed->view_console, c);
            col++;
        }
    }
    if (haveLine)
        UI_endLine (ed->view_console);
    UI_moveCursor(ed->view_console, ed->con_rows, col+1);
}

static void _readline_repaint(UI_view view, char *buf, int16_t cursor_pos, int16_t *scroll_offset, int16_t row, int16_t col, int16_t width)
{
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

    UI_beginLine (view, row, col, width);
    int16_t l = strlen(buf);
    for (uint16_t c = 0; c<width; c++)
    {
        int16_t bp = c+*scroll_offset;
        if (bp<l)
            UI_putc(view, buf[bp]);
        else
            UI_putc (view, ' ');
    }
    UI_endLine (view);
    UI_moveCursor (view, row, col+cp);
}

void IDE_readline (IDE_instance ed, char *buf, int16_t buf_len)
{
    UI_view view = ed->view_console;

    if (!UI_isViewVisible (view))
        UI_setViewVisible (view, TRUE);

    uint16_t view_cols, view_rows;
    UI_getViewSize (view, &view_rows, &view_cols);

    int16_t scroll_offset=0;
    int16_t cursor_pos=0;

    uint16_t col, row;
    UI_getCursorPos (view, &row, &col);

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
                    _readline_repaint(view, buf, cursor_pos, &scroll_offset, row, col, view_cols-col);
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
                    _readline_repaint(view, buf, cursor_pos, &scroll_offset, row, col, view_cols-col);
                }
                else
                {
                    UI_bell();
                }
                break;

            case KEY_HOME:
                cursor_pos = 0;
                _readline_repaint(view, buf, cursor_pos, &scroll_offset, row, col, view_cols-col);
                break;

            case KEY_END:
                cursor_pos = strlen(buf);
                _readline_repaint(view, buf, cursor_pos, &scroll_offset, row, col, view_cols-col);
                break;

            case KEY_BACKSPACE:
                if (cursor_pos>0)
                {
                    uint16_t l = strlen(buf);
                    for (uint16_t i=cursor_pos; i<l; i++)
                        buf[i-1] = buf[i];
                    buf[l-1] = 0;
                    cursor_pos--;
                    _readline_repaint(view, buf, cursor_pos, &scroll_offset, row, col, view_cols-col);
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
                    _readline_repaint(view, buf, cursor_pos, &scroll_offset, row, col, view_cols-col);
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
                    _readline_repaint(view, buf, cursor_pos, &scroll_offset, row, col, view_cols-col);
                }
                else
                {
                    UI_bell();
                }
                break;
            }
        }
    }
}

