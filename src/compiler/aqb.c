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
#include "link.h"

#define VERSION "0.7.0"

static void print_usage(char *argv[])
{
	fprintf(stderr, "usage: %s [ options ] <program.bas>\n", argv[0]);
    fprintf(stderr, "    -d <module>  load <module> implicitly, default: \"_aqb\", specify \"none\" to disable\n");
	fprintf(stderr, "    -L <dir>     look in <dir> for modules\n");
	fprintf(stderr, "    -O           enable optimizer\n");
	fprintf(stderr, "    -a <foo.s>   create gas source file\n");
	fprintf(stderr, "    -A <foo.s>   create ASMOne/ASMPro source file\n");
	fprintf(stderr, "    -s <foo.sym> create symbol file\n");
	fprintf(stderr, "    -o <foo>     create hunk binary file\n");
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
    static char            binfn[PATH_MAX];
    static string          module_name;

#ifdef __amigaos__
    check_stacksize();
#endif

    U_init();
    Ty_init();
    EM_init();
    S_symbol_init();

#ifdef __amigaos__
    E_addModulePath("AQB:lib");
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
                E_addModulePath(argv[optind]);
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
        	case 'o':
                optind++;
                if (optind >= argc)
                {
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                strncpy (binfn, argv[optind], PATH_MAX);
                write_bin = TRUE;
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

        E_addModulePath(dirname(String(sourcefn)));
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

    if (!write_asm && !write_bin)
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

    if (write_asm)
    {
        if (strlen(asm_gas_fn)>0)
        {
            FILE *out = fopen(asm_gas_fn, "w");
            CG_writeASMFile (out, frags, AS_dialect_gas);
            fclose(out);
        }

        if (strlen(asm_asmpro_fn)>0)
        {
            FILE *out = fopen(asm_asmpro_fn, "w");
            CG_writeASMFile (out, frags, AS_dialect_ASMPro);
            fclose(out);
        }
    }

    if (!write_bin)
        exit(0);

    /*
     * machine code generation (assembly phase)
     */

    AS_object obj = AS_Object();

    for (CG_fragList fl=frags; fl; fl=fl->tail)
    {
        CG_frag frag = fl->head;
        switch (frag->kind)
        {
            case CG_procFrag:
            {
                Temp_label   label   = frag->u.proc.label;
                //CG_frame     frame   = frag->u.proc.frame;
                AS_instrList body    = frag->u.proc.body;
                bool         expt    = frag->u.proc.expt;

                if (OPT_get(OPTION_VERBOSE))
                {
                    fprintf(stdout, "\n************************************************************************************************\n");
                    fprintf(stdout, "**\n");
                    fprintf(stdout, "** machine code generation for %s\n", Temp_labelstring(label));
                    fprintf(stdout, "**\n");
                    fprintf(stdout, "************************************************************************************************\n\n");
                    U_memstat();
                }
                if (!AS_assembleCode (obj, body, expt))
                    exit(19);
                break;
            }
            case CG_stringFrag:
                AS_assembleDataAlign2 (obj);
                if (!AS_assembleString (obj, frag->u.stringg.label, frag->u.stringg.str, frag->u.stringg.msize))
                    exit(20);
                break;
            case CG_dataFrag:
                if (!frag->u.data.size)
                    break;
                AS_assembleDataAlign2 (obj);
                if (!AS_assembleDataLabel (obj, frag->u.data.label, frag->u.data.expt))
                    exit(21);
                if (frag->u.data.init)
                {
                    assert(FALSE); // FIXME: implement
#if 0
                    for (CG_dataFragNode n=frag->u.data.init; n; n=n->next)
                    {
                        switch (n->kind)
                        {
                            case CG_labelNode:
                                fprintf(out, "%s:\n", Temp_labelstring(n->u.label));
                                break;
                            case CG_constNode:
                            {
                                Ty_const c = n->u.c;
                                switch (c->ty->kind)
                                {
                                    case Ty_bool:
                                    case Ty_byte:
                                    case Ty_ubyte:
                                        fprintf(out, "    dc.b %d\n", c->u.b);
                                        break;
                                    case Ty_uinteger:
                                    case Ty_integer:
                                        fprintf(out, "    dc.w %d\n", c->u.i);
                                        break;
                                    case Ty_long:
                                    case Ty_ulong:
                                    case Ty_pointer:
                                        fprintf(out, "    dc.l %d\n", c->u.i);
                                        break;
                                    case Ty_single:
                                        fprintf(out, "    dc.l %d /* %f */\n", encode_ffp(c->u.f), c->u.f);
                                        break;
                                    case Ty_string:
                                        fprintf(out, "    .ascii \"%s\"\n", expand_escapes(c->u.s));
                                        break;
                                    case Ty_sarray:
                                    case Ty_darray:
                                    case Ty_record:
                                    case Ty_void:
                                    case Ty_forwardPtr:
                                    case Ty_prc:
                                    case Ty_procPtr:
                                    case Ty_toLoad:
                                    case Ty_double:
                                        assert(0);
                                        break;
                                }
                                break;
                            }
                        }
                    }
#endif
                }
                else
                {
                    AS_assembleDataFill (obj, frag->u.data.size);
                }

                break;
            default:
                assert(FALSE); // FIXME
        }
    }

    AS_resolveLabels (obj);

    /*
     * machine code generation (link phase)
     */

    LI_segmentList sl = LI_SegmentList();

    FILE *fObj = E_openModuleFile ("startup.o");
    if (!fObj)
    {
        fprintf (stderr, "*** ERROR: failed to open startup.o\n\n");
        exit(23);
    }
    if (!LI_segmentListReadObjectFile (sl, fObj))
    {
        fclose(fObj);
        exit(24);
    }
    fclose(fObj);

    if (obj->codeSeg)
        LI_segmentListAppend (sl, obj->codeSeg);
    if (obj->dataSeg)
        LI_segmentListAppend (sl, obj->dataSeg);

    // FIXME: unfinished, hardcoded
    fObj = E_openModuleFile ("_brt.a");
    if (!fObj)
    {
        fprintf (stderr, "*** ERROR: failed to open _brt.a\n\n");
        exit(25);
    }
    if (!LI_segmentListReadObjectFile (sl, fObj))
    {
        fclose(fObj);
        exit(26);
    }
    fclose(fObj);

    if (!LI_link (sl))
    {
        fprintf (stderr, "*** ERROR: failed to link.\n\n");
        exit(27);
    }

    FILE *fLoadFile = fopen(binfn, "w");
    if (!fLoadFile)
    {
        fprintf (stderr, "*** ERROR: failed to open %s for writing.\n\n", binfn);
        exit(28);
    }
    if (!LI_segmentListWriteLoadFile (sl, fLoadFile))
    {
        fclose(fLoadFile);
        exit(29);
    }
    fclose(fLoadFile);

    return 0;
}

