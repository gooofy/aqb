#include "mod2.h"

#include <exec/types.h>
//#include <exec/memory.h>
//#include <clib/exec_protos.h>
//#include <inline/exec.h>

//#include <intuition/intuition.h>
//#include <intuition/intuitionbase.h>
//#include <clib/intuition_protos.h>
//#include <inline/intuition.h>
//
//#include <clib/graphics_protos.h>
//#include <inline/graphics.h>
//
//#include <clib/gadtools_protos.h>
//#include <inline/gadtools.h>

//#include <clib/dos_protos.h>
//#include <inline/dos.h>

mod2_t    _g_m2[MOD2_NUM];

void _mod2_init(void)
{
    for (int i=0; i<MOD2_NUM; i++)
    {
        mod2_t *p = &_g_m2[i];

        p->s = 42;
        p->b = i==0;

    }
}

