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

#include "parser.h"
#include "prabsyn.h"
#include "frame.h"
#include "semant.h"
#include "errormsg.h"
#include "canon.h"
#include "printtree.h"
#include "codegen.h"
#include "regalloc.h"

#define VERSION "0.1.0"

/* print the assembly language instructions to filename.s */
static void doProc(FILE *out, F_frame frame, T_stm body)
{
    AS_proc proc;
    T_stmList stmList;
    AS_instrList iList;

    F_tempMap = Temp_empty();
    //printStmList(stdout, T_StmList(body, NULL));

    stmList = C_linearize(body);
    stmList = C_traceSchedule(C_basicBlocks(stmList));
    fprintf(stdout, ">>>>>>>>>>>>>>>>>>>>> Proc stmt list\n");
    printStmList(stdout, stmList);
    fprintf(stdout, "<<<<<<<<<<<<<<<<<<<<< Proc stmt list\n");

    iList  = F_codegen(frame, stmList); /* 9 */
    // fprintf(stdout, ">>>>>>>>>>>>>>>>>>>>> AS stmt list\n");
    // AS_printInstrList (out, iList, Temp_layerMap(F_tempMap,Temp_name()));
    // fprintf(stdout, "<<<<<<<<<<<<<<<<<<<<< AS stmt list\n");

    struct RA_result ra = RA_regAlloc(frame, iList);  /* 10, 11 */
    iList = ra.il;

    proc = F_procEntryExitAS(frame, iList);

    fprintf(stdout, ">>>>>>>>>>>>>>>>>>>>> AS stmt list\n");
    fprintf(stdout, "%s\n", proc->prolog);
    AS_printInstrList(stdout, proc->body, Temp_layerMap(F_tempMap, Temp_layerMap(ra.coloring, Temp_name())));
    fprintf(stdout, "%s\n", proc->epilog);
    fprintf(stdout, "<<<<<<<<<<<<<<<<<<<<< AS stmt list\n");

    fprintf(out, "%s\n", proc->prolog);
    AS_printInstrList(out, proc->body, Temp_layerMap(F_tempMap, Temp_layerMap(ra.coloring, Temp_name())));
     fprintf(out, "%s\n", proc->epilog);
//  fprintf(out, "BEGIN function\n");
//  AS_printInstrList (out, iList,
//                     Temp_layerMap(F_tempMap, Temp_layerMap(ra.coloring, Temp_name())));
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

int main (int argc, char *argv[])
{
	char           *sourcefn;
	FILE           *sourcef;
    A_sourceProgram sourceProgram;
    F_fragList      frags, fl;
    char            asmfn[1024];
    FILE           *out;

    printf("AQB V" VERSION "\n");

	if (argc != 2)
	{
		fprintf(stderr, "usage: %s <program.bas>\n\n", argv[0]);
		exit(1);
	}

	sourcefn = argv[1];
    /* filename.bas -> filename.s */
    {
        int l = strlen(sourcefn);
        if (l>1024)
            l = 1024;
        if (l<4)
            l = 4;
        strncpy(asmfn, sourcefn, 1024);
        asmfn[l-3] = 's';
        asmfn[l-2] = 0;
    }

    /*
     * parsing
     */

	sourcef = fopen(sourcefn, "r");
	if (!sourcef) 
	{
		fprintf(stderr, "failed to read %s: %s\n\n", sourcefn, strerror(errno));
		exit(2);
	}

	if (!P_sourceProgram(sourcef, sourcefn, &sourceProgram))
        exit(3);

    if (EM_anyErrors)
    {
        printf ("\n\nparing failed.\n");
        exit(4);
    }

    printf ("\n\nparsing worked.\n");

    pr_sourceProgram(stdout, sourceProgram, 0);

	fclose(sourcef);

    /*
     * intermediate code
     */

    F_initRegisters();

    frags = SEM_transProg(sourceProgram);
    if (EM_anyErrors) 
        exit(4);

    printf ("\n\nsemantics worked.\n");

    F_printtree(stdout, frags);

    /*
     * generate target assembly code
     */

    out = fopen(asmfn, "w");

    fprintf(out, ".globl __aqb_main\n\n");
    /* Chapter 8, 9, 10, 11 & 12 */
    fprintf(out, ".text\n\n");
    for (fl=frags; fl; fl=fl->tail)
    {
        if (fl->head->kind == F_procFrag) 
        {
            doProc(out, fl->head->u.proc.frame, fl->head->u.proc.body);
        }
    }

    fprintf(out, ".data\n\n");
    for (fl=frags; fl; fl=fl->tail)
    {
        if (fl->head->kind == F_stringFrag) 
        {
            doStr(out, fl->head->u.stringg.str, fl->head->u.stringg.label);
        }
    }
    fclose(out);

    return 0;
}

