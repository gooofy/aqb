#include "link.h"



static bool fread_u4(FILE *f, uint32_t *u)
{
    if (fread (u, 4, 1, f) != 1)
        return FALSE;
}

void LI_load(FILE *f)
{


}


