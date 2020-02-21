#ifndef HAVE_AWINDOW_H
#define HAVE_AWINDOW_H

#include "autil.h"

#define AW_FLAG_SIZE     1
#define AW_FLAG_DRAG     2
#define AW_FLAG_DEPTH    4
#define AW_FLAG_CLOSE    8
#define AW_FLAG_REFRESH 16

BOOL AW_open(short id, char *title, short x1, short y1, short x2, short y2, short flags, short scr_id);

void AW_shutdown(void);

#endif

