#ifndef HAVE_OPTIONS_H
#define HAVE_OPTIONS_H

#include "util.h"
#include <limits.h>

/*
 * shell environment
 */

extern char aqb_home[PATH_MAX];
extern char aqb_lib[PATH_MAX];
extern char aqb_help[PATH_MAX];
extern bool aqb_wbstart;

/*
 * compiler options
 */

//#define OPTION_EXPLICIT  1      // basic's OPTION EXPLICIT
#define OPTION_VERBOSE   2      // verbose compiler output
//#define OPTION_PRIVATE   4      // set PRIVATE by default
#define OPTION_BREAK     8      // add CTRL-C checks in loops and subprograms
#define OPTION_DEBUG    16      // generate debug code and information

void OPT_set         (int opt, bool onoff);
bool OPT_get         (int opt);
void OPT_reset       (void);                // reset all compiler options to their defaults

/*
 * (persistent) user preferences
 */

//#define OPT_PREF_COLORSCHEME  2
//
//int  OPT_prefGetInt  (int pref);
//void OPT_prefSetInt  (int pref, int i);

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

//#define AQB_MAIN_NAME  "__aqb_main"
//#define AQB_CLEAR_NAME "__aqb_clear"

//#define OPT_DEFAULT_MODULE "_aqb"

// global compiler command line options

//extern string OPT_default_module;
extern string OPT_bin_fn;
extern string OPT_asm_gas_fn;
extern string OPT_asm_asmpro_fn;

void OPT_init(void);
void OPT_deinit(void);

#endif
