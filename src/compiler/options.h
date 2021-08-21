#ifndef HAVE_OPTIONS_H
#define HAVE_OPTIONS_H

#include "util.h"
#include <limits.h>

/*
 * shell environment
 */

extern char aqb_home[PATH_MAX];
extern char aqb_lib[PATH_MAX];

/*
 * command line options
 */

#define OPTION_EXPLICIT  1
#define OPTION_VERBOSE   2
#define OPTION_PRIVATE   4
#define OPTION_RACOLOR   8

void OPT_set           (int opt, bool onoff);
bool OPT_get           (int opt);

/*
 * (persistent) user preferences
 */

#define OPT_PREF_FONT         1
#define OPT_PREF_COLORSCHEME  2

int  OPT_prefGetInt  (int pref);
void OPT_prefSetInt  (int pref, int i);

/*
 * module search path
 */

typedef struct OPT_dirSearchPath_ *OPT_dirSearchPath;

struct OPT_dirSearchPath_
{
    string            path;
    OPT_dirSearchPath next;
};

void              OPT_addModulePath (string path);          /* look for symbol files in directory <path>                   */
OPT_dirSearchPath OPT_getModulePath (void);

#define AQB_MAIN_NAME "__aqb_main"

#define OPT_DEFAULT_MODULE "_aqb"

extern string OPT_default_module;

void OPT_init(void);

#endif
