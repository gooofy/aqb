#ifndef HAVE_OPTIONS_H
#define HAVE_OPTIONS_H

#include "util.h"
#include <limits.h>

/*
 * shell environment
 */

extern char acs_home[PATH_MAX];
extern char acs_lib[PATH_MAX];
//extern char acs_help[PATH_MAX];
extern bool acs_wbstart;

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
 * assembly search path
 */

typedef struct OPT_dirSearchPath_ *OPT_dirSearchPath;

struct OPT_dirSearchPath_
{
    string            path;
    OPT_dirSearchPath next;
};

void              OPT_assemblyAddPath  (string path);       // look for assembly files in directory <path
OPT_dirSearchPath OPT_assemblyGetPath  (void);
FILE             *OPT_assemblyOpenFile (string filename);   // look through search paths, try to open <filename>

//#define AQB_CLEAR_NAME "__acs_clear"

//#define OPT_DEFAULT_MODULE "_acs"

// global compiler command line options

extern string   OPT_sym_fn;
extern string   OPT_bin_fn;
extern string   OPT_asm_gas_fn;
extern string   OPT_asm_asmpro_fn;
extern string   OPT_asm_vasm_fn;
extern string   OPT_binfn;
extern string   OPT_objfn;
extern bool     OPT_hasCode;
extern uint32_t OPT_stackSize;
extern bool     OPT_gcScanExtern;

void OPT_init(void);
void OPT_deinit(void);

#endif
