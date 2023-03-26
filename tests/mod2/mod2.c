#include "mod2.h"

#define ENABLE_DPRINTF
#include "_brt.h"

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

mod2_t    _g_m2_1[MOD2_NUM];
mod2_t    _g_m2_2[MOD2_NUM];

void _mod2_init(void)
{
    for (int i=0; i<MOD2_NUM; i++)
    {
        mod2_t *p = &_g_m2_1[i];
        ULONG *pUL2 = &p->ul2;

        p->ul1 = 42;
        p->s = 43;
        *pUL2 = 0xdeadbeef;
        //DPRINTF("_g_m2_1[%d]: p=0x%08lx, pUL2=0x%08lx\n", i, p, pUL2);
    }
    for (int i=0; i<MOD2_NUM; i++)
    {
        mod2_t *p = &_g_m2_1[i];
        DPRINTF("_g_m2_1[%d]: ul1=0x%08lx, ul2=0x%08lx\n", i, p->ul1, p->ul2);
    }
}

