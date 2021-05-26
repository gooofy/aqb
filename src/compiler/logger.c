#include "logger.h"
#include <stdlib.h>

LOG_cb_t LOG_cb = NULL;

void LOG_init (LOG_cb_t cb)
{
    LOG_cb = cb;
}
