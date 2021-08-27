#include <string.h>

#include "tui.h"
#include "ui.h"
#include "logger.h"

TUI_window TUI_Window (char *title, uint16_t w, uint16_t h)
{
    TUI_window win = (TUI_window) U_poolAlloc (UP_ide, sizeof (*win));

    win->title            = title;
    win->w                = w;
    win->h                = h;

    win->x                = UI_size_cols/2-w/2;
    win->y                = UI_size_rows/2-h/2;

    win->first            = NULL;
    win->last             = NULL;
    win->focus            = NULL;

    win->ok_cb            = NULL;
    win->ok_user_data     = 0;
    win->cancel_cb        = NULL;
    win->cancel_user_data = 0;

    return win;
}

void TUI_addWidget (TUI_window win, TUI_widget widget)
{
    if (win->last)
    {
        win->last->next = widget;
        win->last = widget;
    }
    else
    {
        win->first = win->last = widget;
        widget->next = NULL;
    }

    widget->x += win->x;
    widget->y += win->y;
}

void TUI_refresh (TUI_window w)
{
    UI_setCursorVisible (FALSE);
    UI_setTextStyle     (UI_TEXT_STYLE_DIALOG);

    // dialog title line
    UI_beginLine (w->y+1, w->x+1, w->w);
    UI_putc('=');
    UI_putc('=');
    UI_putc(' ');
    UI_putstr (w->title);
    UI_putc(' ');
    for (uint16_t c = 4 + strlen(w->title); c<w->w; c++)
        UI_putc ('=');
    UI_endLine ();

    for (uint16_t r = 1; r<w->h; r++)
    {
        UI_beginLine (w->y+r+1, w->x+1, w->w);
        for (uint16_t c = 0; c<w->w; c++)
            UI_putc (' ');
        UI_endLine ();
    }

    UI_endLine ();

    // draw widgets

    for (TUI_widget widget = w->first; widget; widget=widget->next)
        widget->refresh_cb(widget);
}

void TUI_focus (TUI_window w, TUI_widget wdgt)
{
    if (w->focus)
    {
        w->focus->focus_cb (w->focus, FALSE);
        w->focus->focused = FALSE;
    }
    w->focus = wdgt;
    if (w->focus)
    {
        w->focus->focus_cb (w->focus, TRUE);
        w->focus->focused = TRUE;
    }
}

void TUI_handleEvent (TUI_window w, uint16_t event)
{
    switch (event)
    {
        case KEY_TAB:
        {
            TUI_widget f = w->focus ? w->focus : w->first;
            do
            {
                f = f->next;
                if (!f)
                    f = w->first;
            } while (!f->focus_cb);
            TUI_focus (w, f);
            break;
        }

        case KEY_ENTER:
            if (w->ok_cb)
                w->ok_cb(NULL, w->ok_user_data);
            break;
        case KEY_ESC:
            if (w->cancel_cb)
                w->cancel_cb(NULL, w->cancel_user_data);
            break;
        default:
            if (w->focus)
                w->focus->event_cb(w->focus, event);
    }
}

void TUI_setOKAction (TUI_window w, TUI_action_cb cb, uint32_t user_data)
{
    w->ok_cb        = cb;
    w->ok_user_data = user_data;
}

void TUI_setCancelAction (TUI_window w, TUI_action_cb cb, uint32_t user_data)
{
    w->cancel_cb        = cb;
    w->cancel_user_data = user_data;
}

/****************************************************************************************
 **
 ** Label widget
 **
 ****************************************************************************************/

typedef struct TUI_label_s *TUI_label;
struct TUI_label_s
{
    struct TUI_widget_s  w;
    char                *label;
};

static void labelRefreshCb(TUI_widget w)
{
    TUI_label label = (TUI_label)w;

    UI_setTextStyle (UI_TEXT_STYLE_DIALOG);

    int16_t l = strlen(label->label);
    UI_beginLine (w->y+1, w->x+1, l);
    UI_putstr (label->label);
    UI_endLine ();
}

TUI_widget TUI_Label (uint16_t x, uint16_t y, char *label)
{
    TUI_label wdgt = (TUI_label) U_poolAlloc (UP_ide, sizeof (*wdgt));

    wdgt->w.w          = strlen(label);
    wdgt->w.h          = 1;
    wdgt->w.x          = x;
    wdgt->w.y          = y;

    wdgt->w.next       = NULL;
    wdgt->w.focused    = FALSE;
    wdgt->w.refresh_cb = labelRefreshCb;
    wdgt->w.focus_cb   = NULL;
    wdgt->w.event_cb   = NULL;

    wdgt->label         = label;

    return &wdgt->w;;
}

/****************************************************************************************
 **
 ** Button widget
 **
 ****************************************************************************************/

typedef struct TUI_button_s *TUI_button;
struct TUI_button_s
{
    struct TUI_widget_s  w;
    char                *label;
    int16_t              label_offset;
    TUI_action_cb        cb;
    uint32_t             user_data;
};

static void buttonRefreshCb(TUI_widget w)
{
    TUI_button button = (TUI_button)w;

    UI_setTextStyle (UI_TEXT_STYLE_TEXT);

    UI_beginLine (w->y+1, w->x+1, w->w);

    int16_t l = strlen(button->label);
    if (w->focused)
        UI_putc ('>');
    else
        UI_putc (' ');
    for (uint16_t c = 1; c<w->w-1; c++)
    {
        int16_t lp = c - button->label_offset;
        if ( (lp>=0) && (lp<l) )
            UI_putc(button->label[lp]);
        else
            UI_putc (' ');
    }
    if (w->focused)
        UI_putc ('<');
    else
        UI_putc (' ');
    UI_endLine ();
}

static void buttonEventCb(TUI_widget w, uint16_t event)
{
    TUI_button button = (TUI_button)w;
    switch (event)
    {
        case 32:
            if (button->cb)
                button->cb(w, button->user_data);
            break;

        case KEY_NONE:
            break;

        default:
        {
            UI_bell();
            break;
        }

    }
}

static void buttonFocusCb(TUI_widget w, bool focus)
{
    w->focused = focus;
    w->refresh_cb(w);
}

TUI_widget TUI_Button (uint16_t x, uint16_t y, uint16_t w, char *label, TUI_action_cb cb, uint32_t user_data)
{
    TUI_button wdgt = (TUI_button) U_poolAlloc (UP_ide, sizeof (*wdgt));

    wdgt->w.w          = w;
    wdgt->w.h          = 1;
    wdgt->w.x          = x;
    wdgt->w.y          = y;

    wdgt->w.next       = NULL;
    wdgt->w.focused    = FALSE;
    wdgt->w.refresh_cb = buttonRefreshCb;
    wdgt->w.focus_cb   = buttonFocusCb;
    wdgt->w.event_cb   = buttonEventCb;

    wdgt->label         = label;
    wdgt->label_offset  = w/2 - strlen(label)/2;
    wdgt->cb            = cb;
    wdgt->user_data     = user_data;

    return &wdgt->w;;
}

/****************************************************************************************
 **
 ** CheckBox widget
 **
 ****************************************************************************************/

typedef struct TUI_checkBox_s *TUI_checkBox;
struct TUI_checkBox_s
{
    struct TUI_widget_s  w;
    char                *label;
    bool                *b;
};

static void checkBoxRefreshCb(TUI_widget w)
{
    TUI_checkBox cb = (TUI_checkBox)w;

    UI_setTextStyle (UI_TEXT_STYLE_TEXT);

    UI_beginLine (w->y+1, w->x+1, w->w);
    //LOG_printf(LOG_DEBUG, "CB w=%d\n", w->w);
    if (w->focused)
    {
        if (!(*cb->b))
            UI_putstr (">[ ]<");
        else
            UI_putstr (">[X]<");
    }
    else
    {
        if (!(*cb->b))
            UI_putstr (" [ ] ");
        else
            UI_putstr (" [X] ");
    }
    UI_putstr (cb->label);
    UI_endLine ();
}

static void checkBoxEventCb(TUI_widget w, uint16_t event)
{
    TUI_checkBox cb = (TUI_checkBox)w;
    switch (event)
    {
        case KEY_NONE:
            break;

        case 32:
            *cb->b = !(*cb->b);
            w->refresh_cb(w);
            break;

        default:
            UI_bell();
    }
}

static void checkBoxFocusCb(TUI_widget w, bool focus)
{
    w->focused = focus;
    w->refresh_cb(w);
}

TUI_widget TUI_CheckBox  (uint16_t x, uint16_t y, uint16_t w, char *label, bool *b)
{
    TUI_checkBox wdgt = (TUI_checkBox) U_poolAlloc (UP_ide, sizeof (*wdgt));

    wdgt->w.w          = w;
    wdgt->w.h          = 1;
    wdgt->w.x          = x;
    wdgt->w.y          = y;

    wdgt->w.next       = NULL;
    wdgt->w.focused    = FALSE;
    wdgt->w.refresh_cb = checkBoxRefreshCb;
    wdgt->w.focus_cb   = checkBoxFocusCb;
    wdgt->w.event_cb   = checkBoxEventCb;

    wdgt->b            = b;
    wdgt->label        = label;

    return &wdgt->w;;
}

/****************************************************************************************
 **
 ** TextEntry widget
 **
 ****************************************************************************************/

typedef struct TUI_textEntry_s *TUI_textEntry;
struct TUI_textEntry_s
{
    struct TUI_widget_s  w;
    char                *buf;
    int16_t              buf_len;
    int16_t              scroll_offset;
    int16_t              cursor_pos;
};

static void textEntryRefreshCb(TUI_widget w)
{
    TUI_textEntry entry = (TUI_textEntry)w;

    UI_setTextStyle (UI_TEXT_STYLE_TEXT);

    int16_t cp=0;
    while (TRUE)
    {
        cp = entry->cursor_pos - entry->scroll_offset;
        if ( (cp>=0) && (cp<w->w) )
            break;
        else if (cp<0)
            entry->scroll_offset--;
        else
            entry->scroll_offset++;
    }

    UI_beginLine (w->y+1, w->x+1, w->w);
    int16_t l = strlen(entry->buf);
    for (uint16_t c = 0; c<w->w; c++)
    {
        int16_t bp = c+entry->scroll_offset;
        if (bp<l)
            UI_putc(entry->buf[bp]);
        else
            UI_putc (' ');
    }
    UI_endLine ();
    if (w->focused)
    {
        //LOG_printf (LOG_DEBUG, "TUI_TextEntry refresh cb: cursor_pos=%d, scroll_offset=%d -> cp=%d\n", entry->cursor_pos, entry->scroll_offset, cp);
        UI_moveCursor (w->y+1, w->x+1+cp);
    }
}

static void textEntryEventCb(TUI_widget w, uint16_t event)
{
    TUI_textEntry entry = (TUI_textEntry)w;
    switch (event)
    {
        case KEY_ESC:
            // FIXME: aktivate cancel button
            break;

        case KEY_CURSOR_LEFT:
            if (entry->cursor_pos>0)
            {
                entry->cursor_pos--;
                textEntryRefreshCb(w);
            }
            else
            {
                UI_bell();
            }
            break;

        case KEY_CURSOR_RIGHT:
            if (entry->cursor_pos<(strlen(entry->buf)))
            {
                entry->cursor_pos++;
                textEntryRefreshCb(w);
            }
            else
            {
                UI_bell();
            }
            break;

        case KEY_HOME:
            entry->cursor_pos = 0;
            textEntryRefreshCb(w);
            break;

        case KEY_END:
            entry->cursor_pos = strlen(entry->buf);
            textEntryRefreshCb(w);
            break;

        case KEY_BACKSPACE:
            if (entry->cursor_pos>0)
            {
                uint16_t l = strlen(entry->buf);
                for (uint16_t i=entry->cursor_pos; i<l; i++)
                    entry->buf[i-1] = entry->buf[i];
                entry->buf[l-1] = 0;
                entry->cursor_pos--;
                textEntryRefreshCb(w);
            }
            else
            {
                UI_bell();
            }
            break;

        case KEY_DEL:
        {
            uint16_t l = strlen(entry->buf);
            if (entry->cursor_pos<l)
            {
                for (uint16_t i=entry->cursor_pos; i<l-1; i++)
                    entry->buf[i] = entry->buf[i+1];
                entry->buf[l-1] = 0;
                textEntryRefreshCb(w);
            }
            else
            {
                UI_bell();
            }
            break;
        }

        case KEY_NONE:
            break;

        default:
        {
            uint16_t l = strlen(entry->buf);
            if ( (event >= 32) && (event <= 126) && (l < entry->buf_len-1) )
            {
                for (int i=l; i>entry->cursor_pos; i--)
                {
                    entry->buf[i]   = entry->buf[i-1];
                }
                entry->buf[entry->cursor_pos] = event;
                entry->buf[l+1] = 0;
                entry->cursor_pos++;
                textEntryRefreshCb(w);
            }
            else
            {
                UI_bell();
            }
            break;
        }

    }
}

static void textEntryFocusCb(TUI_widget w, bool focus)
{
    w->focused = focus;
    if (focus)
    {
        UI_setCursorVisible (TRUE);
        textEntryRefreshCb(w);
    }
    else
    {
        UI_setCursorVisible (FALSE);
    }
}

TUI_widget TUI_TextEntry (uint16_t x, uint16_t y, uint16_t w, char *buf, uint16_t buf_len)
{
    TUI_textEntry wdgt = (TUI_textEntry) U_poolAlloc (UP_ide, sizeof (*wdgt));

    wdgt->w.w          = w;
    wdgt->w.h          = 1;
    wdgt->w.x          = x;
    wdgt->w.y          = y;

    wdgt->w.next       = NULL;
    wdgt->w.focused    = FALSE;
    wdgt->w.refresh_cb = textEntryRefreshCb;
    wdgt->w.focus_cb   = textEntryFocusCb;
    wdgt->w.event_cb   = textEntryEventCb;

    wdgt->buf           = buf;
    wdgt->buf_len       = buf_len;
    wdgt->scroll_offset = 0;
    wdgt->cursor_pos    = strlen(buf);

    return &wdgt->w;;
}

/****************************************************************************************
 **
 ** Find requester
 **
 ****************************************************************************************/

static bool findreq_running;
static bool findreq_result;

static void findButtonCB(TUI_widget w, uint32_t user_data)
{
    findreq_running = FALSE;
    findreq_result  = user_data != 0;
}

bool TUI_FindReq (char *buf, uint16_t buf_len, bool *matchCase, bool *wholeWord, bool *searchBackwards)
{
    TUI_window dlg = TUI_Window ("Find", 60, 14);

    TUI_widget entry = TUI_TextEntry (2, 2, 56, buf, buf_len);
    TUI_addWidget (dlg, entry);

    TUI_widget checkBox = TUI_CheckBox (2, 5, 27, "Match Upper/Lowercase", matchCase);
    TUI_addWidget (dlg, checkBox);

    checkBox = TUI_CheckBox (2, 7, 27, "Whole Word", wholeWord);
    TUI_addWidget (dlg, checkBox);

    checkBox = TUI_CheckBox (2, 9, 27, "Search Backwards", searchBackwards);
    TUI_addWidget (dlg, checkBox);

    TUI_widget okButton = TUI_Button (10, 12, 10, "OK", findButtonCB, 1);
    TUI_addWidget (dlg, okButton);

    TUI_widget cancelButton = TUI_Button (40, 12, 10, "Cancel", findButtonCB, 0);
    TUI_addWidget (dlg, cancelButton);

    TUI_setOKAction     (dlg, findButtonCB, 1);
    TUI_setCancelAction (dlg, findButtonCB, 0);

    TUI_refresh (dlg);
    TUI_focus   (dlg, entry);

    findreq_running = TRUE;
    while (findreq_running)
    {
        uint16_t key = UI_waitkey ();
        TUI_handleEvent (dlg, key);
    }

    return findreq_result;
}

/****************************************************************************************
 **
 ** EZ requester
 **
 ****************************************************************************************/

static bool     ezreq_running;
static uint16_t ezreq_result;

static void ezButtonCB(TUI_widget w, uint32_t user_data)
{
    ezreq_running = FALSE;
    ezreq_result  = user_data;
}

static uint32_t ezWidgetCode (uint16_t i)
{
    uint32_t code;
    switch (i)
    {
        case 0:
            code = 1;
            break;
        case 1:
            code = 0;
            break;
        default:
            code = i;
    }
    return code;
}

uint16_t TUI_EZRequest (char *body, char *gadgets)
{
    uint16_t cnt_lines=1;
    char *c = body;
    while (*c)
    {
        if (*c=='\n')
            cnt_lines++;
        c++;
    }
    cnt_lines++;

    TUI_window dlg = TUI_Window ("AQB Requester", 60, cnt_lines+5);

    /*
     * text labels
     */

    static char buf2[1024];
    strncpy (buf2, body, 1024);
    char *s = buf2;
    c = buf2;
    uint16_t i=0;
    while (*c)
    {
        if (*c=='\n')
        {
            *c = 0;
            TUI_widget label = TUI_Label (2, 2+i, s);
            TUI_addWidget (dlg, label);
            i++;
            c++;
            s=c;
        }
        else
        {
            c++;
        }
    }
    *c = 0;
    TUI_widget label = TUI_Label (2, 2+i, s);
    TUI_addWidget (dlg, label);

    /*
     * buttons
     */

    uint16_t cnt_buttons=1;
    c = gadgets;
    while (*c)
    {
        if (*c=='|')
            cnt_buttons++;
        c++;
    }

    uint16_t gadget_width = 56 / cnt_buttons;
    uint16_t x = 2;
    static char buf[256];
    strncpy (buf, gadgets, 256);
    s = buf;
    c = buf;
    i = 0;
    while (*c)
    {
        if (*c=='|')
        {
            *c = 0;
            uint16_t w = strlen(s);

            TUI_widget button = TUI_Button (x+gadget_width/2-(w+2)/2, cnt_lines+3, w+2, s, ezButtonCB, ezWidgetCode(i));
            TUI_addWidget (dlg, button);
            if (i==0)
                TUI_focus (dlg, button);
            i++;
            c++;
            s=c;
            x+=gadget_width;
        }
        else
        {
            c++;
        }
    }
    *c = 0;
    uint16_t w = strlen(s);
    TUI_widget button = TUI_Button (x+gadget_width/2-(w+2)/2, cnt_lines+3, w+2, s, ezButtonCB, ezWidgetCode(i));
    TUI_addWidget (dlg, button);
    if (i==0)
        TUI_focus (dlg, button);

    TUI_setOKAction     (dlg, ezButtonCB, 1);
    TUI_setCancelAction (dlg, ezButtonCB, 0);

    TUI_refresh (dlg);

    ezreq_running = TRUE;
    while (ezreq_running)
    {
        uint16_t key = UI_waitkey ();
        TUI_handleEvent (dlg, key);
    }

    return ezreq_result;
}

/****************************************************************************************
 **
 ** Help browser
 **
 ****************************************************************************************/

static bool     help_running;

static void helpButtonCB(TUI_widget w, uint32_t user_data)
{
    help_running = FALSE;
}

void TUI_HelpBrowser (void)
{
    TUI_window dlg = TUI_Window ("AQB Help Browser", 60, 23);

    TUI_widget label = TUI_Label (2, 2, "HELP TEXT HERE");
    TUI_addWidget (dlg, label);

    TUI_widget button = TUI_Button (2, 21, 12, "Close", helpButtonCB, 1);
    TUI_addWidget (dlg, button);
    TUI_focus (dlg, button);

    TUI_setOKAction (dlg, helpButtonCB, 1);

    TUI_refresh (dlg);

    help_running = TRUE;
    while (help_running)
    {
        uint16_t key = UI_waitkey ();
        TUI_handleEvent (dlg, key);
    }
}

