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

string OPT_default_module = OPT_DEFAULT_MODULE;

static OPT_dirSearchPath g_moduleSP=NULL, g_moduleSPLast=NULL;

void OPT_addModulePath(string path)
{
    OPT_dirSearchPath p = U_poolAlloc (UP_env, sizeof(*p));

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
