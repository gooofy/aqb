#include <stdlib.h>
#include <libgen.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#include "compiler.h"
#include "codegen.h"
#include "env.h"
#include "frontend.h"
#include "regalloc.h"
#include "errormsg.h"
#include "options.h"
#include "link.h"

void CO_compile(string sourcefn, string symfn, string binfn, string asm_gas_fn, string asm_asmpro_fn)
{
    static CG_fragList     frags;
	static FILE           *sourcef;
    static string          module_name;

    // init environment
    AS_init();
    CG_init();
    E_init();

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

	frags = FE_sourceProgram(sourcef, sourcefn, /*is_main=*/!symfn, module_name);
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

    if (symfn)
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

    if (!asm_gas_fn && !asm_asmpro_fn && !binfn)
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

    if (asm_gas_fn)
    {
        FILE *out = fopen(asm_gas_fn, "w");
        CG_writeASMFile (out, frags, AS_dialect_gas);
        fclose(out);
    }

    if (asm_asmpro_fn)
    {
        FILE *out = fopen(asm_asmpro_fn, "w");
        CG_writeASMFile (out, frags, AS_dialect_ASMPro);
        fclose(out);
    }

    if (!binfn)
        exit(0);

    /*
     * machine code generation (assembly phase)
     */

    AS_object obj = AS_Object(binfn);

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
                    AS_assembleDataFill (obj->dataSeg, frag->u.data.size);
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
    if (!LI_segmentListReadObjectFile (sl, "startup.o", fObj))
    {
        fclose(fObj);
        exit(24);
    }
    fclose(fObj);

    if (obj->codeSeg)
        LI_segmentListAppend (sl, obj->codeSeg);
    if (obj->dataSeg)
        LI_segmentListAppend (sl, obj->dataSeg);

    for (E_moduleListNode n = E_getLoadedModuleList(); n; n=n->next)
    {
        static char mod_fn[PATH_MAX];
        snprintf (mod_fn, PATH_MAX, "%s.a", S_name (n->m->name));
        fObj = E_openModuleFile (mod_fn);
        if (!fObj)
        {
            fprintf (stderr, "*** ERROR: failed to open %s\n\n", mod_fn);
            exit(25);
        }
        if (!LI_segmentListReadObjectFile (sl, S_name(n->m->name), fObj))
        {
            fclose(fObj);
            exit(26);
        }
        fclose(fObj);
    }

    if (!LI_link (sl))
    {
        fprintf (stderr, "*** ERROR: failed to link.\n\n");
        exit(27);
    }

    LI_segmentListWriteLoadFile (sl, binfn);

}

