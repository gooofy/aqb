#ifndef HAVE_OPTIONS_H
#define HAVE_OPTIONS_H

#include "util.h"

#define OPTION_EXPLICIT 1
#define OPTION_VERBOSE  2

void OPT_set(int opt, bool onoff);
bool OPT_get(int opt);

#define AQB_MAIN_NAME "__aqb_main"

#endif
