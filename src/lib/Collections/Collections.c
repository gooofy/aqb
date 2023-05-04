#include "Collections.h"

void _Collections_shutdown(void)
{
}

void _Collections_init(void)
{
    // module initialization - called from __aqb_main

    ON_EXIT_CALL(_Collections_shutdown);

}


