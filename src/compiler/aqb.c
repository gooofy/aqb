/*
 * AQB: AmigaQuickBasic
 *
 * (C) 2020 by G. Bartsch
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

#include "frontend.h"
#include "frame.h"
#include "errormsg.h"
#include "canon.h"
#include "printtree.h"
#include "codegen.h"
#include "regalloc.h"
#include "options.h"
#include "env.h"

#define VERSION "0.5.0"

/* print the assembly language instructions to filename.s */
static void doProc(FILE *out, Temp_label label, bool globl, F_frame frame, T_stm body)
{
    AS_proc proc;
    T_stmList stmList;
    AS_instrList iList;

    //printStmList(stdout, T_StmList(body, NULL));

    stmList = C_linearize(body);
    if (OPT_get(OPTION_VERBOSE))
    {
        fprintf(stdout, ">>>>>>>>>>>>>>>>>>>>> Proc stmt list (after C_linearize)\n");
        printStmList(stdout, stmList);
        fprintf(stdout, "<<<<<<<<<<<<<<<<<<<<< Proc stmt list\n");
    }

    stmList = C_traceSchedule(C_basicBlocks(stmList));

    if (OPT_get(OPTION_VERBOSE))
    {
        fprintf(stdout, ">>>>>>>>>>>>>>>>>>>>> Proc stmt list (after C_traceSchedule)\n");
        printStmList(stdout, stmList);
        fprintf(stdout, "<<<<<<<<<<<<<<<<<<<<< Proc stmt list\n");
    }

    iList  = F_codegen(frame, stmList);

    if (OPT_get(OPTION_VERBOSE))
    {
        fprintf(stdout, ">>>>>>>>>>>>>>>>>>>>> AS stmt list after codegen, before regalloc:\n");
        AS_printInstrList (stdout, iList, Temp_getNameMap());
        fprintf(stdout, "<<<<<<<<<<<<<<<<<<<<< AS stmt list\n");
    }

    struct RA_result ra = RA_regAlloc(frame, iList);
    iList = ra.il;

    proc = F_procEntryExitAS(frame, iList);

    if (OPT_get(OPTION_VERBOSE))
    {
        fprintf(stdout, ">>>>>>>>>>>>>>>>>>>>> AS stmt list\n");
        fprintf(stdout, "%s\n", proc->prolog);
        AS_printInstrList(stdout, proc->body, Temp_layerMap(ra.coloring, Temp_getNameMap()));
        fprintf(stdout, "%s\n", proc->epilog);
        fprintf(stdout, "<<<<<<<<<<<<<<<<<<<<< AS stmt list\n");
    }

    if (globl)
        fprintf(out, ".globl %s\n\n", S_name(label));
    fprintf(out, "%s\n", proc->prolog);
    AS_printInstrList(out, proc->body, Temp_layerMap(ra.coloring, Temp_getNameMap()));
    fprintf(out, "%s\n", proc->epilog);
//  fprintf(out, "BEGIN function\n");
//  AS_printInstrList (out, iList,
//                     Temp_layerMap(ra.coloring, Temp_getNameMap()));
//  fprintf(out, "END function\n\n");
}

char *expand_escapes(const char* src)
{
    char *str = checked_malloc(2 * strlen(src) + 10);

    char *dest = str;
    char c;

    while ((c = *(src++)))
    {
        switch(c)
        {
            case '\a':
                *(dest++) = '\\';
                *(dest++) = 'a';
                break;
            case '\b':
                *(dest++) = '\\';
                *(dest++) = 'b';
                break;
            case '\t':
                *(dest++) = '\\';
                *(dest++) = 't';
                break;
            case '\n':
                *(dest++) = '\\';
                *(dest++) = 'n';
                break;
            case '\v':
                *(dest++) = '\\';
                *(dest++) = 'v';
                break;
            case '\f':
                *(dest++) = '\\';
                *(dest++) = 'f';
                break;
            case '\r':
                *(dest++) = '\\';
                *(dest++) = 'r';
                break;
            case '\\':
                *(dest++) = '\\';
                *(dest++) = '\\';
                break;
            case '\"':
                *(dest++) = '\\';
                *(dest++) = '\"';
                break;
            default:
                *(dest++) = c;
         }
    }

    *(dest++) = '\\';
    *(dest++) = '0';

    *(dest++) = '\0'; /* Ensure nul terminator */
    return str;
}

static void doStr(FILE * out, string str, Temp_label label) {
    fprintf(out, "    .align 4\n");
    fprintf(out, "%s:\n", Temp_labelstring(label));
    fprintf(out, "    .ascii \"%s\"\n", expand_escapes(str));
    fprintf(out, "\n");
}

static void doData(FILE * out, Temp_label label, bool globl, int size, unsigned char *data)
{
    fprintf(out, "    .align 4\n");
    if (globl)
        fprintf(out, ".globl %s\n\n", Temp_labelstring(label));
    fprintf(out, "%s:\n", Temp_labelstring(label));
    if (data)
    {
        int i;
        switch(size)
        {
            case 1:
                fprintf(out, "    dc.b %d\n", data[0]);
                break;
            case 2:
                fprintf(out, "    dc.b %d, %d\n", data[1], data[0]);
                break;
            case 4:
                fprintf(out, "    dc.b %d, %d, %d, %d\n", data[3], data[2], data[1], data[0]);
                break;
            default:
                fprintf(out, "    dc.b");
                for (i=0; i<size; i++)
                {
                    fprintf(out, "%d", data[i]);
                    if (i<size-1)
                        fprintf(out, ",");
                }
                fprintf(out, "    \n");
                break;
        }

    }
    else
    {
        fprintf(out, "    .fill %d\n", size);
    }
    fprintf(out, "\n");
}

static void print_usage(char *argv[])
{
	fprintf(stderr, "usage: %s [-v] [-s] <program.bas>\n", argv[0]);
    fprintf(stderr, "    -d <module> load <module> implicitly, default: \"_aqb\", specify \"none\" to disable\n");
	fprintf(stderr, "    -L <dir>    look in <dir> for symbol files\n");
	fprintf(stderr, "    -s          create symbol (.sym) file\n");
	fprintf(stderr, "    -S          create symbol (.sym) file only (do not create assembly file)\n");
	fprintf(stderr, "    -v          verbose\n");
	fprintf(stderr, "    -V          display version info\n");
}

int main (int argc, char *argv[])
{
	char           *sourcefn;
	FILE           *sourcef;
    F_fragList      frags, fl;
    char            asmfn[PATH_MAX];
    FILE           *out;
    size_t 			optind;
    bool            write_sym = FALSE;
    bool            no_asm = FALSE;
    char            symfn[PATH_MAX];
    string          module_name;

    Ty_init();
    EM_init();
    S_symbol_init();

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
        	case 's':
                write_sym = TRUE;
				break;
        	case 'S':
                write_sym = TRUE;
                no_asm    = TRUE;
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
    F_initRegisters();
    E_init();

	sourcefn = argv[optind];
    /* filename.bas -> filename.s, filename.sym, module name, module search path */
    {
        int l = strlen(sourcefn);
        if (l>1024)
            l = 1024;
        if (l<4)
            l = 4;
        strncpy(asmfn, sourcefn, PATH_MAX-1);
        asmfn[l-3] = 's';
        asmfn[l-2] = 0;
        strncpy(symfn, sourcefn, PATH_MAX-1);
        symfn[l-3] = 's';
        symfn[l-2] = 'y';
        symfn[l-1] = 'm';
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
        F_printtree(stdout, frags);
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

    if (no_asm)
        exit(0);

    /*
     * generate target assembly code
     */

    out = fopen(asmfn, "w");

    fprintf(out, ".text\n\n");
    for (fl=frags; fl; fl=fl->tail)
    {
        if (fl->head->kind == F_procFrag)
        {
            doProc(out, fl->head->u.proc.label, fl->head->u.proc.globl, fl->head->u.proc.frame, fl->head->u.proc.body);
        }
    }

    fprintf(out, ".data\n\n");
    for (fl=frags; fl; fl=fl->tail)
    {
        if (fl->head->kind == F_stringFrag)
        {
            doStr(out, fl->head->u.stringg.str, fl->head->u.stringg.label);
        }
        if (fl->head->kind == F_dataFrag)
        {
            doData(out, fl->head->u.data.label, fl->head->u.data.globl, fl->head->u.data.size, fl->head->u.data.init);
        }
    }
    fclose(out);

    return 0;
}

