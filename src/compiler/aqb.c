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
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>

#ifdef __amigaos__
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

#define VERSION "0.7.0"

static void print_usage(char *argv[])
{
	fprintf(stderr, "usage: %s [ options ] <program.bas>\n", argv[0]);
    fprintf(stderr, "    -d <module>  load <module> implicitly, default: \"_aqb\", specify \"none\" to disable\n");
	fprintf(stderr, "    -L <dir>     look in <dir> for symbol files\n");
	fprintf(stderr, "    -O           enable optimizer\n");
	fprintf(stderr, "    -a <foo.s>   create gas source file\n");
	fprintf(stderr, "    -A <foo.s>   create ASMOne/ASMPro source file\n");
	fprintf(stderr, "    -s <foo.sym> create symbol file\n");
	fprintf(stderr, "    -v           verbose\n");
	fprintf(stderr, "    -V           display version info\n");
}

#ifdef __amigaos__

#define MIN_STACKSIZE 64*1024

static void check_stacksize(void)
{
    struct Process *Process;
    struct CommandLineInterface *CLI;
    ULONG stack;

    Process = (struct Process *) FindTask (0L);
    if ( (CLI = (struct CommandLineInterface *) (Process -> pr_CLI << 2)) )
    {
        stack = CLI -> cli_DefaultStack << 2;
    }
    else
    {
        stack = Process -> pr_StackSize;
    }
    if (stack < MIN_STACKSIZE)
    {
        fprintf (stderr, "*** error: current stack size of %ld bytes is too small for this program, need at least %d bytes.\n", stack, MIN_STACKSIZE);
        exit(EXIT_FAILURE);
    }
}
#endif

int main (int argc, char *argv[])
{
	static char           *sourcefn;
	static FILE           *sourcef;
    static CG_fragList     frags;
    static char            asm_gas_fn[PATH_MAX];
    static char            asm_asmpro_fn[PATH_MAX];
    static size_t          optind;
    static bool            write_sym = FALSE;
    static bool            write_asm = FALSE;
    static bool            write_bin = FALSE;
    static char            symfn[PATH_MAX];
    static string          module_name;

#ifdef __amigaos__
    check_stacksize();
#endif

    U_init();
    Ty_init();
    EM_init();
    S_symbol_init();

#ifdef __amigaos__
    E_addSymPath("AQB:lib");
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
                E_addSymPath(argv[optind]);
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
                write_asm = TRUE;
				break;
        	case 'A':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                strncpy (asm_asmpro_fn, argv[optind], PATH_MAX);
                write_asm = TRUE;
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
				break;
        	case 'v':
				OPT_set(OPTION_VERBOSE, TRUE);
				break;
        	case 'V':
                fprintf (stderr, "AQB V" VERSION " (C) 2020 by G. Bartsch.\nLicensed under the Apache License, Version 2.0.\n");
                exit(0);
				break;
        	default:
				print_usage(argv);
	            exit(EXIT_FAILURE);
        }
    }

	if (argc != (optind+1))
	{
		print_usage(argv);
		exit(EXIT_FAILURE);
	}

    // init environment
    AS_init();
    CG_init();
    E_init();

	sourcefn = argv[optind];
    /* filename.bas -> filename.s, filename.sym, module name, module search path */
    {
        int l = strlen(sourcefn);
        if (l>1024)
            l = 1024;
        if (l<4)
            l = 4;

        if (strlen(asm_gas_fn)==0)
        {
            strncpy(asm_gas_fn, sourcefn, PATH_MAX-1);
            asm_gas_fn[l-3] = 's';
            asm_gas_fn[l-2] = 0;
        }

        // strncpy(symfn, sourcefn, PATH_MAX-1);
        // symfn[l-3] = 's';
        // symfn[l-2] = 'y';
        // symfn[l-1] = 'm';
        module_name = basename(String(sourcefn));
        l = strlen(module_name);
        module_name[l-4] = 0;

        E_addSymPath(dirname(String(sourcefn)));
    }

    /*
     * frontend: parsing + semantics
     */

	sourcef = fopen(sourcefn, "r");
	if (!sourcef)
	{
		fprintf(stderr, "failed to read %s: %s\n\n", sourcefn, strerror(errno));
		exit(2);
	}

	frags = FE_sourceProgram(sourcef, sourcefn, !write_sym, module_name);
	fclose(sourcef);

    if (EM_anyErrors)
    {
        printf ("\n\nfrontend processing failed - exiting.\n");
        exit(4);
    }

    if (OPT_get(OPTION_VERBOSE))
    {
        printf ("\n\nsemantics worked.\n");
        U_memstat();
    }

    /*
     * generate symbol file
     */

    if (write_sym)
    {
        if (FE_writeSymFile(symfn))
        {
            if (OPT_get(OPTION_VERBOSE))
                printf ("\n%s written.\n", symfn);
        }
        else
        {
            printf ("\n** ERROR: failed to write symbol file %s .\n", symfn);
            exit(23);
        }
    }

    if (!write_asm)
        exit(0);

    /*
     * register allocation
     */

    for (CG_fragList fl=frags; fl; fl=fl->tail)
    {
        CG_frag frag = fl->head;
        if (frag->kind != CG_procFrag)
            continue;

        Temp_label   label   = frag->u.proc.label;
        CG_frame     frame   = frag->u.proc.frame;
        AS_instrList body    = frag->u.proc.body;

        if (OPT_get(OPTION_VERBOSE))
        {
            fprintf(stdout, "\n************************************************************************************************\n");
            fprintf(stdout, "**\n");
            fprintf(stdout, "** register allocation for %s\n", Temp_labelstring(label));
            fprintf(stdout, "**\n");
            fprintf(stdout, "************************************************************************************************\n\n");
            fprintf(stdout, ">>>>>>>>>>>>>>>>>>>>> Proc %s AS stmt list after codegen, before regalloc:\n", Temp_labelstring(label));
            AS_printInstrList (stdout, body, AS_dialect_gas);
            fprintf(stdout, "<<<<<<<<<<<<<<<<<<<<< Proc %s AS stmt list after codegen, before regalloc.\n", Temp_labelstring(label));
            U_memstat();
        }

        if (!RA_regAlloc(frame, body) || EM_anyErrors)
        {
            printf ("\n\nregister allocation failed - exiting.\n");
            exit(24);
        }

        CG_procEntryExitAS(frag);

        if (OPT_get(OPTION_VERBOSE))
        {
            fprintf(stdout, ">>>>>>>>>>>>>>>>>>>>> Proc %s AS stmt list (after CG_procEntryExitAS):\n", Temp_labelstring(label));
            AS_printInstrList(stdout, body, AS_dialect_gas);
            fprintf(stdout, "<<<<<<<<<<<<<<<<<<<<< Proc %s AS stmt list (after CG_procEntryExitAS).\n", Temp_labelstring(label));
            U_memstat();
        }
    }

    /*
     * generate target assembly code
     */

    FILE *out = fopen(asm_gas_fn, "w");
    CG_writeASMFile (out, frags, AS_dialect_gas);
    fclose(out);

    if (strlen(asm_asmpro_fn)>0)
    {
        FILE *out = fopen(asm_asmpro_fn, "w");
        CG_writeASMFile (out, frags, AS_dialect_ASMPro);
        fclose(out);
    }

    if (!write_bin)
        exit(0);

    /*
     * machine code generation (assembly phase)
     */

    AS_segment seg_code = AS_CodeSegment();

    for (CG_fragList fl=frags; fl; fl=fl->tail)
    {
        CG_frag frag = fl->head;
        if (frag->kind != CG_procFrag)
            continue;

        Temp_label   label   = frag->u.proc.label;
        //CG_frame     frame   = frag->u.proc.frame;
        AS_instrList body    = frag->u.proc.body;

        if (OPT_get(OPTION_VERBOSE))
        {
            fprintf(stdout, "\n************************************************************************************************\n");
            fprintf(stdout, "**\n");
            fprintf(stdout, "** machine code generation for %s\n", Temp_labelstring(label));
            fprintf(stdout, "**\n");
            fprintf(stdout, "************************************************************************************************\n\n");
            U_memstat();
        }
        AS_assemble (seg_code, body);
    }

    return 0;
}

