#ifndef HAVE_LOGGER_H
#define HAVE_LOGGER_H

#include <inttypes.h>

#define LOG_DEBUG  0
#define LOG_INFO   1
#define LOG_ERROR  2

#define LOG_LEVEL LOG_DEBUG
//#define LOG_LEVEL LOG_INFO

void runlogger (uint8_t lvl, char *fmt, ...);

#define LOG_printf(lvl, ...) do { if (lvl >= LOG_LEVEL) runlogger(lvl, __VA_ARGS__); } while (0)

#endif

