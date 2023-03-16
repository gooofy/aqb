#ifndef HAVE_MINRT_H
#define HAVE_MINRT_H

#include <exec/types.h>

/*
 * minrt: minimalistic AQB runtime
 *
 * part of commonrt, used both by the compiler/ide and all AQB programs
 */

/*
 * CTRL-C / CTRL-D (DEBUG) BREAK handling
 *
 * minrt installs an input handler (via input.device) which will (just)
 * the set _break_status global variable when a CTRL-C/CTRL-D input
 * event occurs.
 *
 * the application needs to check for this condition at strategic locations
 * regularly and call __handle_break() or handle break conditions internally.
 * The CHKBRK macro is provided for convenience.
 *
 * __handle_break() will call the break_handler first if one is registered.
 * if this returns FALSE or no break_handler is registered, __handle_break()
 * will either generate a debugger TRAP if a debugger is connected or exit()
 * otherwise.
 *
 */

#define BREAK_CTRL_C    1
#define BREAK_CTRL_D    2

extern USHORT _break_status = 0;

#define CHKBRK if (_break_status) __handle_break()
void __handle_break(void);

#endif

