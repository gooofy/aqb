#ifndef HAVE_UI_H
#define HAVE_UI_H

// (pretty simplistic) amiga / linux UI abstraction layer

#include "util.h"

#define UI_MIN_COLUMNS       10
#define UI_MAX_COLUMNS       256
#define UI_MIN_ROWS          5
#define UI_MAX_ROWS          256

#define KEY_ESC              27
#define KEY_CTRL_A            1
#define KEY_CTRL_B            2
#define KEY_CTRL_C            3
#define KEY_CTRL_D            4
#define KEY_CTRL_E            5
#define KEY_CTRL_F            6
#define KEY_CTRL_G            7
#define KEY_BACKSPACE         8
#define KEY_TAB               9
#define KEY_CTRL_L           12
#define KEY_ENTER            13
#define KEY_CTRL_N           14
#define KEY_CTRL_O           15
#define KEY_CTRL_Q           17
#define KEY_CTRL_S           19
#define KEY_CTRL_U           21
#define KEY_CTRL_Y           25
#define KEY_DEL             127
#define KEY_CURSOR_LEFT    1000
#define KEY_CURSOR_RIGHT   1001
#define KEY_CURSOR_UP      1002
#define KEY_CURSOR_DOWN    1003
#define KEY_HOME           1005
#define KEY_END            1006
#define KEY_PAGE_UP        1007
#define KEY_PAGE_DOWN      1008
#define KEY_F1             1010
#define KEY_F2             1011
#define KEY_F3             1012
#define KEY_F4             1013
#define KEY_F5             1014
#define KEY_F6             1015
#define KEY_F7             1016
#define KEY_F8             1017
#define KEY_F9             1018
#define KEY_F10            1019
#define KEY_HELP           1020
#define KEY_CLOSE          1021
#define KEY_GOTO_EOF       1022
#define KEY_GOTO_BOF       1023
#define KEY_COLORSCHEME_0  1024
#define KEY_COLORSCHEME_1  1025
#define KEY_COLORSCHEME_2  1026
#define KEY_COLORSCHEME_3  1027
#define KEY_COLORSCHEME_4  1028
#define KEY_COLORSCHEME_5  1029
#define KEY_ABOUT          1030
#define KEY_FONT_0         1031
#define KEY_FONT_1         1032
#define KEY_FIND           1033
#define KEY_FIND_NEXT      1034
#define KEY_BLOCK          1035
#define KEY_NEW            1036
#define KEY_OPEN           1037
#define KEY_SAVE           1038
#define KEY_SAVE_AS        1039
#define KEY_QUIT           1040
#define KEY_UNKNOWN1       9993
#define KEY_UNKNOWN2       9994
#define KEY_UNKNOWN3       9995
#define KEY_UNKNOWN4       9996
#define KEY_UNKNOWN5       9997
#define KEY_UNKNOWN6       9998
#define KEY_NONE           9999

#define UI_TEXT_STYLE_TEXT     0
#define UI_TEXT_STYLE_KEYWORD  1
#define UI_TEXT_STYLE_COMMENT  2
#define UI_TEXT_STYLE_INVERSE  3
#define UI_TEXT_STYLE_DIALOG   4

void      UI_setTextStyle       (uint16_t style);
void      UI_beginLine          (uint16_t row, uint16_t col_start, uint16_t cols);
void      UI_putc               (char c);
void      UI_putstr             (string s);
void      UI_printf             (char* format, ...);
void      UI_vprintf            (char* format, va_list ap);
void      UI_endLine            (void);

void      UI_setCursorVisible   (bool visible);
void      UI_moveCursor         (uint16_t row, uint16_t col);
uint16_t  UI_waitkey            (void);
void      UI_bell               (void);
void      UI_eraseDisplay       (void);

void      UI_setColorScheme     (int scheme);
void      UI_setFont            (int font);

void      UI_setScrollArea      (uint16_t row_start, uint16_t row_end);
void      UI_scrollUp           (bool fullscreen);
void      UI_scrollDown         (void);

extern uint16_t UI_size_cols, UI_size_rows;

typedef void (*UI_size_cb)(void *user_data);
void      UI_onSizeChangeCall   (UI_size_cb cb, void *user_data);

typedef void (*UI_key_cb)(uint16_t key, void *user_data);
void      UI_onKeyCall          (UI_key_cb cb, void *user_data);

/*
 * high-level requesters
 */

uint16_t  UI_EZRequest          (char *body, char *gadgets, ...);
char     *UI_FileReq            (char *title);
bool      UI_FindReq            (char *buf, uint16_t buf_len, bool *matchCase, bool *wholeWord, bool *searchBackwards);

#ifdef __amigaos__
struct FileHandle *UI_output    (void);
int       UI_termSignal         (void);
void      UI_runIO              (void);
#endif

bool      UI_init               (void);
void      UI_deinit             (void);

void      UI_run                (void);

#endif

