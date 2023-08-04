#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "options.h"
#include "logger.h"

#ifdef __amigaos__
#include <clib/dos_protos.h>
#include <inline/dos.h>
extern struct DOSBase       *DOSBase;
#endif

#define DEFAULT_OPTS ( OPTION_BREAK | OPTION_DEBUG )

//static char g_pref_fn[PATH_MAX];
//static int  g_pref_colorscheme  = 0;
static int  g_opt               = DEFAULT_OPTS;
//static bool g_changed           = false;

string OPT_bin_fn        = NULL;
string OPT_asm_gas_fn    = NULL;
string OPT_asm_asmpro_fn = NULL;
string OPT_asm_vasm_fn   = NULL;
string OPT_binfn         = NULL;
string OPT_objfn         = NULL;
bool   OPT_hasCode       = true;

void OPT_set(int opt, bool onoff)
{
    if (onoff)
        g_opt |= opt;
    else
        g_opt &= ~opt;
}

bool OPT_get(int opt)
{
    return g_opt & opt;
}

void OPT_reset (void)
{
    g_opt = DEFAULT_OPTS;
    //g_changed = true;
}

//int OPT_prefGetInt (int pref)
//{
//    switch (pref)
//    {
//        case OPT_PREF_COLORSCHEME:
//            return g_pref_colorscheme;
//    }
//
//    assert(false);
//    return false;
//}
//
//void OPT_prefSetInt (int pref, int i)
//{
//    switch (pref)
//    {
//        case OPT_PREF_COLORSCHEME:
//            g_pref_colorscheme = i;
//            break;
//        default:
//            assert(false);
//    }
//    g_changed = true;
//}

// string OPT_default_module = OPT_DEFAULT_MODULE;

static OPT_dirSearchPath g_assemblySP=NULL, g_assemblySPLast=NULL;

void OPT_assemblyAddPath (string path)
{
    OPT_dirSearchPath p = U_poolAlloc (UP_options, sizeof(*p));

    p->path      = String(UP_options, path);
    p->next      = NULL;

    if (g_assemblySP)
    {
        g_assemblySPLast->next = p;
        g_assemblySPLast = p;
    }
    else
    {
        g_assemblySP = g_assemblySPLast = p;
    }
}

OPT_dirSearchPath OPT_getModulePath (void)
{
    return g_assemblySP;
}

FILE *OPT_assemblyOpenFile (string filename)
{
    for (OPT_dirSearchPath sp=OPT_getModulePath(); sp; sp=sp->next)
    {
        char modfn[PATH_MAX];

        snprintf(modfn, PATH_MAX, "%s/%s", sp->path, filename);

        LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "trying to load %s from %s ...\n", filename, modfn);

        FILE *f = fopen(modfn, "r");
        if (f)
        {
            LOG_printf (LOG_DEBUG, "%s opened for reading\n", modfn);
            return f;
        }
    }
    return NULL;
}

void OPT_init(void)
{
    // read prefs from disk

//#ifdef __amigaos__
//    strncpy (g_pref_fn, aqb_home, PATH_MAX);
//    AddPart ((STRPTR)g_pref_fn, (STRPTR)"prefs.ini", PATH_MAX);
//    // printf ("aqb_home='%s' aqb_lib='%s' g_pref_fn='%s'\n",
//    //         aqb_home, aqb_lib, g_pref_fn);
//    // for (int i = 0; i<5; i++)
//    //     printf ("%c [%d] ", aqb_home[i], aqb_home[i]);
//    // printf ("\n");
//    // for (int i = 0; i<5; i++)
//    //     printf ("%c [%d] ", g_pref_fn[i], g_pref_fn[i]);
//    // printf ("\n");
//#else
//    if (snprintf (g_pref_fn, PATH_MAX, "%s/prefs.ini", aqb_home)<0)
//    {
//        fprintf (stderr, "prefs.ini path too long\n");
//        exit(EXIT_FAILURE);
//    }
//#endif
//
//    //printf ("options: trying to read prefs from %s\n", g_pref_fn);
//
//    FILE *prefFile = fopen(g_pref_fn, "r");
//    if (prefFile)
//    {
//        char line[1024];
//        while (fgets (line, 1024, prefFile))
//        {
//            char *equals = strchr (line, '=');
//            if (!equals)
//            {
//                printf ("options: ignoring prefs line %s\n", line);
//                continue;
//            }
//            char *value = equals+1;
//            *equals=0;
//            if (!strcmp(line, "colorscheme"))
//            {
//                int i;
//                if (str2int (value, &i))
//                {
//                    g_pref_colorscheme = i;
//                }
//                else
//                {
//                    printf ("options: failed to parse value '%s'\n", value);
//                }
//            }
//            else
//            {
//                printf ("options: unknown prefs option '%s'\n", line);
//            }
//        }
//        fclose(prefFile);
//    }
//    //printf ("options: read prefs from %s.\n", g_pref_fn);
}

void OPT_deinit(void)
{
//    LOG_printf (LOG_DEBUG, "OPT_deinit g_pref_fn=%s\n", g_pref_fn);
//    if (g_changed)
//    {
//        FILE *prefFile = fopen(g_pref_fn, "w");
//        if (!prefFile)
//        {
//            LOG_printf (LOG_ERROR, "failed to open %s for writing\n", g_pref_fn);
//            return;
//        }
//        fprintf (prefFile, "colorscheme=%d\n", g_pref_colorscheme);
//        fclose(prefFile);
//    }
//    LOG_printf (LOG_DEBUG, "OPT_deinit done\n");
}

