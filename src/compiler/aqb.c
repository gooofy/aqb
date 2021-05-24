/*
 * AQB: AmigaQuickBasic
 *
 * (C) 2020, 2021 by G. Bartsch
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>

#ifdef __amigaos__

#include "stabs.h"

#include <exec/types.h>
#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;

#endif

#include "frontend.h"
#include "errormsg.h"
#include "codegen.h"
#include "regalloc.h"
#include "options.h"
#include "env.h"
#include "link.h"
#include "ide.h"
#include "compiler.h"
#include "logger.h"

#define VERSION "0.7.0"

static void print_usage(char *argv[])
{
	fprintf(stderr, "usage: %s [ options ] <program.bas>\n", argv[0]);
    fprintf(stderr, "    -d <module>  load <module> implicitly, default: \"_aqb\", specify \"none\" to disable\n");
	fprintf(stderr, "    -L <dir>     look in <dir> for modules\n");
	fprintf(stderr, "    -O           enable optimizer\n");
	fprintf(stderr, "    -a <foo.s>   create gas source file\n");
	fprintf(stderr, "    -A <foo.s>   create ASMOne/ASMPro source file\n");
	fprintf(stderr, "    -B <foo.s>   create vasm source file\n");
	fprintf(stderr, "    -s <foo.sym> create symbol file\n");
	fprintf(stderr, "    -o <foo>     create hunk binary file\n");
	fprintf(stderr, "    -p <foo>     create hunk object file\n");
	fprintf(stderr, "    -v           verbose\n");
	fprintf(stderr, "    -V           display version info\n");
}

#ifdef __amigaos__

#define MIN_STACKSIZE 64*1024

unsigned long __stack=MIN_STACKSIZE;

extern struct WBStartup *_WBenchMsg;

void __request (string msg);

static void check_amigaos_env(void)
{
    /*
     * check minimum library versions
     */

    if ( ((struct Library *)DOSBase)->lib_Version < 38)
    {
        __request ("DOS library is too old, need at least V38");
        exit(1);
    }

    /*
     * check stack size
     */

    struct Process *pr;
    struct CommandLineInterface *CLI;
    ULONG stack;

    pr = (struct Process *) FindTask (0L);
    if ( (CLI = (struct CommandLineInterface *) (pr->pr_CLI << 2)) )
    {
        stack = CLI->cli_DefaultStack << 2;
        printf ("1\n");
    }
    else
    {
        stack = pr->pr_StackSize;
    }

	ULONG size;
	size = (char *)pr->pr_Task.tc_SPUpper - (char *)pr->pr_Task.tc_SPLower;
	if (pr->pr_CLI)
		size = *(ULONG *)pr->pr_ReturnAddr;

    printf ("pr->pr_StackSize=%ld, size=%ld\n", pr->pr_StackSize, size);
    if (stack < MIN_STACKSIZE)
    {
        static char buf[255];
        snprintf (buf, 255, "stack size of %ld bytes is too small, need at least %d bytes.\n", stack, MIN_STACKSIZE);
        __request(buf);
        exit(EXIT_FAILURE);
    }
}
#endif

#if LOG_LEVEL == LOG_DEBUG
static FILE *logf=NULL;
#endif

static void log_cb (uint8_t lvl, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    static char buf[1024];
    vsnprintf (buf, 1024, fmt, args);
    va_end(args);
	if (lvl >= LOG_INFO)
    {
        printf ("%s", buf);
        fflush (stdout);
    }
#if LOG_LEVEL == LOG_DEBUG
	fprintf (logf, "%s", buf);
	fflush (logf);
#endif
}

static void deinit(void)
{
    U_deinit();
#if LOG_LEVEL == LOG_DEBUG
	fclose (logf);
#endif
}

int main (int argc, char *argv[])
{
	static string sourcefn;
    static string module_name;
    static char   asm_gas_fn[PATH_MAX];
    static char   asm_asmpro_fn[PATH_MAX];
    static char   asm_vasm_fn[PATH_MAX];
    static int    optind;
    static bool   write_sym = FALSE;
    static bool   write_asmpro = FALSE;
    static bool   write_vasm = FALSE;
    static bool   write_asmgas = FALSE;
    static bool   write_obj = FALSE;
    static bool   write_bin = FALSE;
    static bool   launch_ide = TRUE;
    static char   symfn[PATH_MAX];
    static char   objfn[PATH_MAX];
    static char   binfn[PATH_MAX];

#ifdef __amigaos__
    check_amigaos_env();
#endif

    U_init();
    SYM_init();
    FE_boot();
    E_boot();

#ifdef __amigaos__
    OPT_addModulePath("AQB:lib");
#else
    char *aqb_env = getenv ("AQB");
    if (aqb_env)
    {
        // FIXME: path does not conform to linux fs hier
        snprintf (symfn, PATH_MAX, "%s/src/lib", aqb_env);
        OPT_addModulePath(symfn);
    }
#endif

    asm_gas_fn[0]=0;
    asm_asmpro_fn[0]=0;

    for (optind = 1; optind < argc && argv[optind][0] == '-'; optind++)
	{
        switch (argv[optind][1])
		{
        	case 'd':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                OPT_default_module = argv[optind];
				break;
        	case 'L':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                OPT_addModulePath(argv[optind]);
				break;
        	case 'O':
				OPT_set(OPTION_RACOLOR, TRUE);
				break;
        	case 'a':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                strncpy (asm_gas_fn, argv[optind], PATH_MAX);
                write_asmgas = TRUE;
                launch_ide = FALSE;
				break;
        	case 'A':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                strncpy (asm_asmpro_fn, argv[optind], PATH_MAX);
                write_asmpro = TRUE;
                launch_ide = FALSE;
				break;
        	case 'B':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                strncpy (asm_vasm_fn, argv[optind], PATH_MAX);
                write_vasm = TRUE;
                launch_ide = FALSE;
				break;
        	case 's':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                strncpy (symfn, argv[optind], PATH_MAX);
                write_sym = TRUE;
                launch_ide = FALSE;
				break;
        	case 'o':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                strncpy (binfn, argv[optind], PATH_MAX);
                write_bin = TRUE;
                launch_ide = FALSE;
				break;
        	case 'p':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                strncpy (objfn, argv[optind], PATH_MAX);
                write_obj = TRUE;
                launch_ide = FALSE;
				break;
        	case 'v':
				OPT_set(OPTION_VERBOSE, TRUE);
				break;
        	case 'V':
                fprintf (stderr, "AQB V" VERSION " (C) 2020, 2021 by G. Bartsch.\nLicensed under the Apache License, Version 2.0.\n");
                exit(0);
				break;
        	default:
				print_usage(argv);
	            exit(EXIT_FAILURE);
        }
    }

    if ((argc == 0) || (argc==optind))  // workbench launch / no args
    {
        launch_ide = TRUE;
        sourcefn = NULL;
    }
    else
    {
        if (argc != (optind+1))
        {
            print_usage(argv);
            exit(EXIT_FAILURE);
        }
        sourcefn = argv[optind];
    }

    // run interactive IDE ? (experimental)
    if (launch_ide)
    {
        IDE_open(sourcefn);
        exit(0);
    }

    /* filename.bas -> module name, module search path */
    {
        int l = strlen(sourcefn);
        if (l>1024)
            l = 1024;
        if (l<4)
            l = 4;

        module_name = basename(String(sourcefn));
        l = strlen(module_name);
        module_name[l-4] = 0;

        OPT_addModulePath(dirname(String(sourcefn)));
    }

    // run compiler from commandline

#if LOG_LEVEL == LOG_DEBUG
    logf = fopen (LOG_FILENAME, "a");
#endif
	atexit (deinit);
    LOG_init (log_cb);

    CO_compile(sourcefn,
               module_name,
               write_sym ? symfn : NULL,
               write_obj ? objfn : NULL,
               write_bin ? binfn : NULL,
               write_asmgas ? asm_gas_fn : NULL,
               write_asmpro ? asm_asmpro_fn : NULL,
               write_vasm ? asm_vasm_fn : NULL);

    return 0;
}

