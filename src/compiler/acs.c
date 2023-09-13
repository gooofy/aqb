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
#include "semantics.h"
//#include "link.h"
#include "compiler.h"
#include "logger.h"

char acs_home[PATH_MAX];
char acs_lib[PATH_MAX];
//char acs_help[PATH_MAX];
bool acs_wbstart = false;

static void print_usage(char *argv[])
{
    fprintf(stderr, "usage: %s [ options ] <assembly-name> <src1.cs> [ <src2.cs> ... ]\n", argv[0]);
    fprintf(stderr, "    -l <assembly> load <assembly> (can be specified more than once)\n");
    fprintf(stderr, "    -L <dir>      look in <dir> for assemblies\n");
    fprintf(stderr, "    -P            print symbol information for each loaded assembly\n");
    fprintf(stderr, "    -a            create gas source file\n");
    fprintf(stderr, "    -A            create ASMOne/ASMPro source file\n");
    fprintf(stderr, "    -B            create vasm source file\n");
    fprintf(stderr, "    -s            create symbol file\n");
    fprintf(stderr, "    -S            create C stub file\n");
    fprintf(stderr, "    -I            interface assembly (no code)\n");
    //fprintf(stderr, "    -N            no not generate a assembly init function\n");
    fprintf(stderr, "    -E            no not generate gc scan functions\n");
    fprintf(stderr, "    -o <foo>      create hunk binary file\n");
    fprintf(stderr, "    -p <foo>      create hunk object file\n");
    fprintf(stderr, "    -T <n>        stack size for the generated program\n");
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

    if (!ASUP_NameFromLock(aqbProc->pr_HomeDir, (STRPTR)acs_home, PATH_MAX))
    {
        U_request (NULL, NULL, "OK", "Failed to determine ACS installation dir: %d", IoErr());
        exit(EXIT_FAILURE);
    }
    //printf ("detected acs_home: %s\n", acs_home);
    strncpy (acs_lib, acs_home, PATH_MAX);
    if (!AddPart ((STRPTR) acs_lib, (STRPTR) "lib", PATH_MAX))
    {
        U_request (NULL, NULL, "OK", "Failed to determine ACS library dir: %d", IoErr());
        exit(EXIT_FAILURE);
    }
    //strncpy (acs_help, acs_home, PATH_MAX);
    //if (!AddPart ((STRPTR) acs_help, (STRPTR) "help", PATH_MAX))
    //{
    //    U_request (NULL, NULL, "OK", "Failed to determine ACS help dir: %d", IoErr());
    //    exit(EXIT_FAILURE);
    //}
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
    IR_namesAddType (sys_names, S_Symbol ("Char"   ), IR_TypeByte()   );
    IR_namesAddType (sys_names, S_Symbol ("SByte"  ), IR_TypeSByte()  );
    IR_namesAddType (sys_names, S_Symbol ("Int16"  ), IR_TypeInt16()  );
    IR_namesAddType (sys_names, S_Symbol ("Int32"  ), IR_TypeInt32()  );
    IR_namesAddType (sys_names, S_Symbol ("Byte"   ), IR_TypeByte()   );
    IR_namesAddType (sys_names, S_Symbol ("UInt16" ), IR_TypeUInt16() );
    IR_namesAddType (sys_names, S_Symbol ("UInt32" ), IR_TypeUInt32() );
    IR_namesAddType (sys_names, S_Symbol ("Boolean"), IR_TypeBoolean());
    IR_namesAddType (sys_names, S_Symbol ("Single" ), IR_TypeSingle() );
    IR_namesAddType (sys_names, S_Symbol ("Double" ), IR_TypeDouble() );

    return root;
}

int main (int argc, char *argv[])
{
    string assembly_name = NULL;
    int    optind;
    //bool   hasCode = true;
    //bool   noInitFn = false;
    //bool   gcScanExtern = false;

#ifdef __amigaos__
    check_amigaos_env();
#else
    char *acs_env = getenv ("ACS");
    if (acs_env)
    {
        strncpy (acs_home, acs_env, PATH_MAX);
        // FIXME: path does not conform to linux fs hier
        snprintf (acs_lib, PATH_MAX, "%s/src/lib", acs_env);
        //printf ("acs_home: %s\n", acs_home);
        //if (snprintf (acs_help, PATH_MAX, "%s/help", acs_home)<0)
        //{
        //    fprintf (stderr, "ACS: failed to compose help path.\n\n");
        //    exit(EXIT_FAILURE);
        //}
    }
    else
    {
        fprintf (stderr, "ACS: env var not set.\n\n");
        exit(EXIT_FAILURE);
    }
#endif

    U_init();
    SYM_init();
    PA_boot();
    SEM_boot();
    OPT_init();

    OPT_assemblyAddPath(acs_lib);

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
                OPT_assembliesAdd (argv[optind]);
                break;
            case 'L':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                OPT_assemblyAddPath(argv[optind]);
                break;
            case 'P':
                OPT_dumpAssemblies = true;
                break;
            case 'I':
                OPT_hasCode = false;
                break;
            //case 'N':
            //    noInitFn = true;
            //    break;
            case 'E':
                OPT_gcScanExtern = true;
                break;
            case 'a':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                OPT_asm_gas_fn = argv[optind];
                break;
            case 'A':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                OPT_asm_asmpro_fn = argv[optind];
                break;
            case 'B':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                OPT_asm_vasm_fn = argv[optind];
                break;
            case 's':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                OPT_sym_fn = argv[optind];
                break;
            case 'S':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                OPT_cstub_fn = argv[optind];
                break;
            case 'o':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                OPT_binfn = argv[optind];
                break;
            case 'p':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                OPT_objfn = argv[optind];
                break;
            case 'T':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                OPT_stackSize = atoi(argv[optind]);
                break;
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

    for (OPT_pathList assemblies = OPT_assembliesGet(); assemblies; assemblies=assemblies->next)
    {
        IR_assembly a = IR_loadAssembly (S_Symbol (assemblies->path), names_root);
        LOG_printf (LOG_INFO, "loading assembly %s\n", assemblies->path);
        if (!a)
        {
            fprintf (stderr, "*** error: failed to load assembly '%s'.\n\n", assemblies->path);
            exit(EXIT_FAILURE);
        }
    }

    //while (optind < argc)
    //{
    //    sourcefn = argv[optind++];

    CO_AssemblyParse (assembly, names_root, argc-optind, &argv[optind]);
    //}

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

