#ifndef HAVE_AQB_H
#define HAVE_AQB_H

#include "../_brt/_brt.h"

extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase       *GfxBase;

#define AW_FLAG_SIZE     1
#define AW_FLAG_DRAG     2
#define AW_FLAG_DEPTH    4
#define AW_FLAG_CLOSE    8
#define AW_FLAG_REFRESH 16

void _awindow_init(void);
void _awindow_shutdown(void);

BOOL  WINDOW(short id, char *title, BOOL s1, short x1, short y1, BOOL s2, short x2, short y2, short flags, short scrid);
BOOL  LINE(BOOL s1, short x1, short y1, BOOL s2, short x2, short y2, short c, short bf);
void  SLEEP(void);
void  ON_WINDOW_CALL(void (*cb)(void));
ULONG WINDOW_(short n);
BOOL  PSET(short x, short y, short color);

#endif

