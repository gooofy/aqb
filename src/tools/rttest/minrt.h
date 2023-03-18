#ifndef HAVE_MINRT_H
#define HAVE_MINRT_H

#include <exec/types.h>
#include <stdbool.h>
#include <stdarg.h>

/*
 * minrt: minimalistic AQB runtime
 *
 * part of commonrt, used both by the compiler/ide and all AQB programs
 */

/*
 * startup and shutdown
 *
 * stdlib's exit(int status) is implemented in startup.s. exit() calls _cexit()
 * which runs the the exit handlers registered via atexit().
 *
 */

// prints msg via debugger, then calls exit(return_code)
void _exit_msg (LONG return_code, UBYTE *msg);

/*
 * stack
 *
 * if _aqb_stack_size in nonzero, a custom stack will be allocated
 * for the application. AllocVec() is done in _cstartup, Exec's StackSwap()
 * is then called from startup.s
 *
 * FIXME: check for current stack size, swap stack only if _aqb_stack_size is 
 *        larger then what we have already
 */

extern ULONG _aqb_stack_size;

/*
 * libraries
 *
 * the following libraries will be opened by minrt
 */

extern struct ExecBase      *SysBase       ;
extern struct DOSBase       *DOSBase       ;
extern struct MathBase      *MathBase      ;
extern struct MathTransBase *MathTransBase ;
extern struct UtilityBase   *UtilityBase   ;

/*
 * debug connection, debug output
 */

#define STARTUP_CLI    1    // started from shell/cli
#define STARTUP_WBENCH 2    // started from workbench
#define STARTUP_DEBUG  3    // started from debugger/ide

extern USHORT _startup_mode;
extern USHORT ERR;          // error code for debug reply msg

void _debugger_init(void);
//void _debug_putc(const char c);
//void _debug_puts(const UBYTE *s);
//void _dprintf(const char *format, ...);

#ifdef ENABLE_DPRINTF

#define DPRINTF(...) printf(__VA_ARGS__)

#else

#define DPRINTF(...)

#endif

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
 * __handle_break() will call the break_handler first if one is registered
 * via atbreak(). If this returns false or no break_handler is registered,
 * __handle_break() will either generate a debugger TRAP if a debugger is
 * connected or exit() otherwise.
 *
 */

#define BREAK_CTRL_C    1
#define BREAK_CTRL_D    2

extern USHORT _break_status;

#define CHKBRK if (_break_status) __handle_break()
void __handle_break(void);

// _atbreak(): add a break handler callback.
//             if the callback returns false -> trap, else continue program
void _atbreak(bool (*cb)(void));

/*
 * stdio subset implementation
 *
 * all *printf() functions rely on vcbprintf() which is callback based
 * the callback is called for each character for actual output
 */

void vcbprintf(const char *format, va_list args, void (*cb)(char c));

#endif

