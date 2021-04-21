#include "logger.h"

LOG_cb_t LOG_cb;

void LOG_init (LOG_cb_t cb)
{
    LOG_cb = cb;
}
