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

