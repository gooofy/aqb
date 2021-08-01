#ifndef HAVE_TUI_H
#define HAVE_TUI_H

#include <inttypes.h>
#include "util.h"

typedef struct TUI_widget_s *TUI_widget;
typedef struct TUI_window_s *TUI_window;
typedef void (*TUI_action_cb) (TUI_widget w, uint32_t user_data);

struct TUI_window_s
{
    char      *title;
    uint16_t   x, y, w, h;
    TUI_widget first, last;
    TUI_widget focus;

    TUI_action_cb  ok_cb;
    uint32_t       ok_user_data;
    TUI_action_cb  cancel_cb;
    uint32_t       cancel_user_data;
};

typedef void (*TUI_refresh_cb)(TUI_widget w);
typedef void (*TUI_focus_cb)  (TUI_widget w, bool focus);
typedef void (*TUI_event_cb)  (TUI_widget w, uint16_t event);

struct TUI_widget_s
{
    TUI_widget     next;
    uint16_t       x, y, w, h;
    bool           focused;
    TUI_refresh_cb refresh_cb;
    TUI_focus_cb   focus_cb;
    TUI_event_cb   event_cb;
};

TUI_window TUI_Window (char *title, uint16_t w, uint16_t h);
void       TUI_addWidget (TUI_window win, TUI_widget widget);

void       TUI_refresh (TUI_window w);
void       TUI_focus   (TUI_window w, TUI_widget wdgt);

void       TUI_handleEvent (TUI_window w, uint16_t event);

void       TUI_setOKAction (TUI_window w, TUI_action_cb cb, uint32_t user_data);
void       TUI_setCancelAction (TUI_window w, TUI_action_cb cb, uint32_t user_data);

/*
 * widgets
 */

TUI_widget TUI_Label     (uint16_t x, uint16_t y, char *label);
TUI_widget TUI_Button    (uint16_t x, uint16_t y, uint16_t w, char *label, TUI_action_cb cb, uint32_t user_data);
TUI_widget TUI_CheckBox  (uint16_t x, uint16_t y, uint16_t w, char *label, bool *b);
TUI_widget TUI_TextEntry (uint16_t x, uint16_t y, uint16_t w, char *buf, uint16_t buf_len);

/*
 * high-level requesters
 */

bool       TUI_FindReq   (char *buf, uint16_t buf_len, bool *matchCase, bool *wholeWord, bool *searchBackwards);
uint16_t   TUI_EZRequest (char *body, char *gadgets);

#endif
