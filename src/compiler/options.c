#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "options.h"
#include "logger.h"

static int g_opt=0;
static char g_pref_fn[PATH_MAX];
static bool g_pref_customscreen = TRUE;
static int  g_pref_colorscheme  = 1;

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

int OPT_prefGetInt (int pref)
{
    switch (pref)
    {
        case OPT_PREF_CUSTOMSCREEN:
            return g_pref_customscreen;
        case OPT_PREF_COLORSCHEME:
            return g_pref_colorscheme;
    }

    assert(FALSE);
    return FALSE;
}

static void save_prefs(void)
{
    FILE *prefFile = fopen(g_pref_fn, "w");
    if (!prefFile)
    {
        LOG_printf (LOG_ERROR, "failed to open %s for writing\n", g_pref_fn);
        return;
    }
    fprintf (prefFile, "customscreen=%d\n", g_pref_customscreen);
    fprintf (prefFile, "colorscheme=%d\n", g_pref_colorscheme);
    fclose(prefFile);
}

void OPT_prefSetInt (int pref, int i)
{
    switch (pref)
    {
        case OPT_PREF_CUSTOMSCREEN:
            g_pref_customscreen = i;
            break;
        case OPT_PREF_COLORSCHEME:
            g_pref_colorscheme = i;
            break;
        default:
            assert(FALSE);
    }
    save_prefs();
}

string OPT_default_module = OPT_DEFAULT_MODULE;

static OPT_dirSearchPath g_moduleSP=NULL, g_moduleSPLast=NULL;

void OPT_addModulePath(string path)
{
    OPT_dirSearchPath p = U_poolAlloc (UP_options, sizeof(*p));

    p->path      = String(path);
    p->next      = NULL;

    if (g_moduleSP)
    {
        g_moduleSPLast->next = p;
        g_moduleSPLast = p;
    }
    else
    {
        g_moduleSP = g_moduleSPLast = p;
    }
}

OPT_dirSearchPath OPT_getModulePath (void)
{
    return g_moduleSP;
}

void OPT_init(void)
{
    // read prefs from disk

#ifdef __amigaos__
    snprintf (g_pref_fn, PATH_MAX, "PROGDIR:prefs.ini");
#else
    char *aqb_env = getenv ("AQB");
    assert(aqb_env);
    snprintf (g_pref_fn, PATH_MAX, "%s/prefs.ini", aqb_env);
#endif

    //printf ("options: trying to read prefs from %s\n", g_pref_fn);

    FILE *prefFile = fopen(g_pref_fn, "r");
    if (prefFile)
    {
        char line[1024];
        while (fgets (line, 1024, prefFile))
        {
            char *equals = strchr (line, '=');
            if (!equals)
            {
                printf ("options: ignoring prefs line %s\n", line);
                continue;
            }
            char *value = equals+1;
            *equals=0;
            if (!strcmp(line, "customscreen"))
            {
                int i;
                if (str2int (value, &i))
                {
                    g_pref_customscreen = i;
                }
                else
                {
                    printf ("options: failed to parse value '%s'\n", value);
                }
            }
            else
            {
                if (!strcmp(line, "colorscheme"))
                {
                    int i;
                    if (str2int (value, &i))
                    {
                        g_pref_colorscheme = i;
                    }
                    else
                    {
                        printf ("options: failed to parse value '%s'\n", value);
                    }
                }
                else
                {
                    printf ("options: unknown prefs option '%s'\n", line);
                }
            }
        }
        fclose(prefFile);
    }

}
