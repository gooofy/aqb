/*
 * ACS: Amiga C#
 *
 * Licensed under the MIT License
 *
 * Copyright (c) 2020, 2021, 2022, 2023 G. Bartsch
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
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

#include <workbench/startup.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;

#include "amigasupport.h"

#endif

#include "parser.h"
#include "errormsg.h"
//#include "codegen.h"
#include "options.h"
//#include "env.h"
//#include "link.h"
#include "compiler.h"
#include "logger.h"

char aqb_home[PATH_MAX];
char aqb_lib[PATH_MAX];
char aqb_help[PATH_MAX];
bool aqb_wbstart = false;

static void print_usage(char *argv[])
{
	fprintf(stderr, "usage: %s [ options ] <assembly-name> <src1.cs> [ <src2.cs> ... ]\n", argv[0]);
    //fprintf(stderr, "    -l <assembly> load <assembly> (can be specified more than once)\n");
	//fprintf(stderr, "    -L <dir>      look in <dir> for assemblies\n");
	//fprintf(stderr, "    -a            create gas source file\n");
	//fprintf(stderr, "    -A            create ASMOne/ASMPro source file\n");
	//fprintf(stderr, "    -B            create vasm source file\n");
	//fprintf(stderr, "    -s            create symbol file\n");
	//fprintf(stderr, "    -S            create C stub file\n");
	//fprintf(stderr, "    -I            interface assembly (no code)\n");
	//fprintf(stderr, "    -N            no not generate a assembly init function\n");
	//fprintf(stderr, "    -E            no not generate a gc scan functions\n");
	//fprintf(stderr, "    -o <foo>      create hunk binary file\n");
	//fprintf(stderr, "    -p <foo>      create hunk object file\n");
	fprintf(stderr, "    -v            verbose\n");
	fprintf(stderr, "    -V            display version info\n");
}

#ifdef __amigaos__

char *versionstr = "\0$VER: " PROGRAM_NAME_SHORT " " VERSION " (" PROGRAM_DATE ")";

#define MIN_STACKSIZE 64*1024

extern struct WBStartup *_WBenchMsg;

static void check_amigaos_env(void)
{
    /*
     * check minimum library versions
     */

    if ( ((struct Library *)DOSBase)->lib_Version < 37)
    {
        U_request (NULL, NULL, "OK", "DOS library V%d is too old, need at least V37", ((struct Library *)DOSBase)->lib_Version);
        exit(EXIT_FAILURE);
    }

    struct Process *aqbProc;
    struct CommandLineInterface *CLI;
    ULONG stack;

    aqbProc = (struct Process *) FindTask (0L);
    if ( (CLI = (struct CommandLineInterface *) (aqbProc->pr_CLI << 2)) )
    {
        stack = CLI->cli_DefaultStack << 2;
    }
    else
    {
        stack = aqbProc->pr_StackSize;
    }
    if (stack < MIN_STACKSIZE)
    {
        U_request (NULL, NULL, "OK", "stack of %ld bytes is too small, need at least %d bytes.", stack, MIN_STACKSIZE);
        exit(EXIT_FAILURE);
    }

    /*
     * get home (installation) directory
     */

    if (!ASUP_NameFromLock(aqbProc->pr_HomeDir, (STRPTR)aqb_home, PATH_MAX))
    {
        U_request (NULL, NULL, "OK", "Failed to determine AQB installation dir: %d", IoErr());
        exit(EXIT_FAILURE);
    }
    //printf ("detected aqb_home: %s\n", aqb_home);
    strncpy (aqb_lib, aqb_home, PATH_MAX);
    if (!AddPart ((STRPTR) aqb_lib, (STRPTR) "lib", PATH_MAX))
    {
        U_request (NULL, NULL, "OK", "Failed to determine AQB library dir: %d", IoErr());
        exit(EXIT_FAILURE);
    }
    strncpy (aqb_help, aqb_home, PATH_MAX);
    if (!AddPart ((STRPTR) aqb_help, (STRPTR) "help", PATH_MAX))
    {
        U_request (NULL, NULL, "OK", "Failed to determine AQB help dir: %d", IoErr());
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
    if (logf)
    {
	    fprintf (logf, "%s", buf);
	    fflush (logf);
    }
#endif
}

static void deinit(void)
{
    OPT_deinit();
    U_deinit();
#if LOG_LEVEL == LOG_DEBUG
	fclose (logf);
    logf = NULL;
#endif
}

static IR_namespace _bootstrap_root_names(void)
{
    IR_namespace root = IR_Namespace(/*name=*/NULL, /*parent=*/NULL);

    IR_namespace sys_names = IR_namesResolveNames (root, S_Symbol ("System"), /*doCreate=*/true);
    IR_namesAddType (sys_names, S_Symbol ("Char"), IR_TypeUByte());

    return root;
}

int main (int argc, char *argv[])
{
	string sourcefn = NULL;
    string assembly_name = NULL;
    int    optind;
    //bool   hasCode = true;
    //bool   noInitFn = false;
    //bool   gcScanExtern = false;
    //string symfn=NULL;
    //string cstubfn=NULL;
    //string objfn=NULL;
    //string binfn=NULL;
    //string asm_gas_fn=NULL;
    //string asm_asmpro_fn=NULL;
    //string asm_vasm_fn=NULL;

#ifdef __amigaos__
    check_amigaos_env();
#else
    char *aqb_env = getenv ("AQB");
    if (aqb_env)
    {
        strncpy (aqb_home, aqb_env, PATH_MAX);
        // FIXME: path does not conform to linux fs hier
        snprintf (aqb_lib, PATH_MAX, "%s/src/lib", aqb_env);
        //printf ("aqb_home: %s\n", aqb_home);
        if (snprintf (aqb_help, PATH_MAX, "%s/help", aqb_home)<0)
        {
            fprintf (stderr, "AQB: failed to compose help path.\n\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        fprintf (stderr, "AQB: env var not set.\n\n");
        exit(EXIT_FAILURE);
    }
#endif

    U_init();
    SYM_init();
    PA_boot();
    // FIXME E_boot();
    OPT_init();

    OPT_addModulePath(aqb_lib);

    for (optind = 1; optind < argc && argv[optind][0] == '-'; optind++)
	{
        switch (argv[optind][1])
		{
        	case 'l':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                assert(false); // FIXME: load assembly
                //OPT_default_module = argv[optind];
				break;
        	//case 'L':
            //    optind++;
            //    if (optind >= argc)
            //    {
            //        print_usage(argv);
            //        exit(EXIT_FAILURE);
            //    }
            //    OPT_addModulePath(argv[optind]);
			//	break;
        	//case 'I':
			//	hasCode = false;
			//	break;
        	//case 'N':
			//	noInitFn = true;
			//	break;
        	//case 'E':
			//	gcScanExtern = true;
			//	break;
        	//case 'a':
            //    optind++;
            //    if (optind >= argc)
            //    {
            //        print_usage(argv);
            //        exit(EXIT_FAILURE);
            //    }
            //    asm_gas_fn = argv[optind];
			//	break;
        	//case 'A':
            //    optind++;
            //    if (optind >= argc)
            //    {
            //        print_usage(argv);
            //        exit(EXIT_FAILURE);
            //    }
            //    asm_asmpro_fn = argv[optind];
			//	break;
        	//case 'B':
            //    optind++;
            //    if (optind >= argc)
            //    {
            //        print_usage(argv);
            //        exit(EXIT_FAILURE);
            //    }
            //    asm_vasm_fn = argv[optind];
			//	break;
        	//case 's':
            //    optind++;
            //    if (optind >= argc)
            //    {
            //        print_usage(argv);
            //        exit(EXIT_FAILURE);
            //    }
            //    symfn = argv[optind];
			//	break;
        	//case 'S':
            //    optind++;
            //    if (optind >= argc)
            //    {
            //        print_usage(argv);
            //        exit(EXIT_FAILURE);
            //    }
            //    cstubfn = argv[optind];
			//	break;
        	//case 'o':
            //    optind++;
            //    if (optind >= argc)
            //    {
            //        print_usage(argv);
            //        exit(EXIT_FAILURE);
            //    }
            //    binfn = argv[optind];
			//	break;
        	//case 'p':
            //    optind++;
            //    if (optind >= argc)
            //    {
            //        print_usage(argv);
            //        exit(EXIT_FAILURE);
            //    }
            //    objfn = argv[optind];
			//	break;
        	case 'v':
				OPT_set(OPTION_VERBOSE, true);
				break;
        	case 'V':
                fprintf (stderr, PROGRAM_NAME_SHORT " " VERSION " " COPYRIGHT "\n" LICENSE "\n");
                exit(0);
				break;
        	default:
				print_usage(argv);
	            exit(EXIT_FAILURE);
        }
    }

    if (argc==optind)
    {
        print_usage(argv);
        exit(EXIT_FAILURE);
    }
    assembly_name = argv[optind++];

#if LOG_LEVEL == LOG_DEBUG
    logf = fopen (LOG_FILENAME, "a");
#endif
	atexit (deinit);
    LOG_init (log_cb);

    IR_assembly assembly    = CO_AssemblyInit (S_Symbol(assembly_name));
    IR_namespace names_root = _bootstrap_root_names();

    while (optind < argc)
    {
        sourcefn = argv[optind++];

        CO_AssemblyParse (assembly, names_root, sourcefn);
    }

    //return CO_compile(sourcefn,
    //                  module_name,
    //                  symfn,
    //                  cstubfn,
    //                  objfn,
    //                  binfn,
    //                  asm_gas_fn,
    //                  asm_asmpro_fn,
    //                  asm_vasm_fn,
    //                  hasCode,
    //                  noInitFn,
    //                  gcScanExtern);
}

