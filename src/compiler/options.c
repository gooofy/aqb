#include <stdlib.h>

#include "options.h"

static int g_opt=0;

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
    // FIXME: just hardcoded defaults, for now
    switch (pref)
    {
        case OPT_PREF_CUSTOMSCREEN:
            return TRUE;
        case OPT_PREF_COLORSCHEME:
            return 0;
    }

    assert(FALSE);
    return FALSE;
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
#else
    char *aqb_env = getenv ("AQB");
    assert(aqb_env);
#endif

}
