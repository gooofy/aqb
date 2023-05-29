#include <stdlib.h>
#include <libgen.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <setjmp.h>

#include "compiler.h"
#include "codegen.h"
#include "env.h"
#include "frontend.h"
#include "linscan.h"
#include "errormsg.h"
#include "options.h"
#include "link.h"
#include "logger.h"

#ifdef __amigaos__
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>

#include <clib/exec_protos.h>

#include <inline/exec.h>

extern struct ExecBase      *SysBase;
#endif

static jmp_buf g_exit_jmp_buf;
static int     g_return_code = 0;

void CO_exit(int return_code)
{
    g_return_code = return_code;
    longjmp (g_exit_jmp_buf, 1);
}

int CO_compile(string sourcefn, string module_name, string symfn, string cstubfn, string objfn, string binfn,
               string asm_gas_fn, string asm_asmpro_fn, string asm_vasm_fn, bool hasCode, bool noInitFn,
               bool gcScanExtern)
{
    static CG_fragList     frags;
	static FILE           *sourcef;

    float startTime = U_getTime();
#ifdef __amigaos__
    U_memstat();
    LOG_printf (LOG_INFO, "\ncompilation starts, %d bytes free...\n\n", AvailMem(MEMF_CHIP) + AvailMem(MEMF_FAST));
#else
    LOG_printf (LOG_INFO, "\ncompilation starts...\n\n");
#endif

    // init environment

    Temp_init();
    Ty_init();
    EM_init();
    FE_init();

    AS_init();
    CG_init();
    E_init();

    if (setjmp(g_exit_jmp_buf))
    {
        U_poolReset (UP_env);
        U_poolReset (UP_codegen);
        U_poolReset (UP_assem);
        U_poolReset (UP_frontend);
        U_poolReset (UP_types);

        U_poolReset (UP_temp);
        U_poolReset (UP_flowgraph);
        U_poolReset (UP_linscan);
        //U_poolReset (UP_symbol);
        U_poolReset (UP_regalloc);
        U_poolReset (UP_liveness);
        //U_poolReset (UP_strings); // FIXME: remove string pool
        U_poolReset (UP_link);

        //E_deInit();
        //CG_deInit();
        //AS_deInit();

        //FE_deInit();
        //EM_deInit();
        //Ty_deInit();

#ifdef __amigaos__
        U_memstat();
        LOG_printf (LOG_INFO, "%d bytes free.\n", AvailMem(MEMF_CHIP) + AvailMem(MEMF_FAST));
#endif
        return g_return_code;
    }

    /*
     * frontend: parsing + semantics
     */

    LOG_printf (LOG_INFO, "PASS 1: parser\n");
	sourcef = fopen(sourcefn, "r");
	if (!sourcef)
	{
		LOG_printf (LOG_ERROR, "failed to read %s: %s\n\n", sourcefn, strerror(errno));
		CO_exit(EXIT_FAILURE);
	}

	frags = FE_sourceProgram(sourcef, sourcefn, /*is_main=*/!symfn, module_name, noInitFn, gcScanExtern);
	fclose(sourcef);

    if (EM_anyErrors)
    {
        LOG_printf (LOG_ERROR, "\n\nfrontend processing failed - exiting.\n");
        CO_exit(EXIT_FAILURE);
    }

    LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "\n\nsemantics worked.\n");
    U_memstat();

    /*
     * generate symbol file
     */

    if (symfn)
    {
        if (FE_writeSymFile(symfn, hasCode))
        {
            LOG_printf (LOG_INFO, "created symbol file: %s\n", symfn);
        }
        else
        {
            LOG_printf (LOG_ERROR, "\n** ERROR: failed to write symbol file %s .\n", symfn);
            CO_exit(EXIT_FAILURE);
        }

        // unresolved forwarded types will result in error messages here
        if (EM_anyErrors)
            CO_exit(EXIT_FAILURE);
    }

    /*
     * generate C stub file
     */

    if (cstubfn)
    {
        if (FE_writeCStubFile(cstubfn))
        {
            LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "\n%s written.\n", cstubfn);
        }
        else
        {
            LOG_printf (LOG_ERROR, "\n** ERROR: failed to write C stub file %s .\n", cstubfn);
            CO_exit(EXIT_FAILURE);
        }

        // unresolved forwarded types will result in error messages here
        if (EM_anyErrors)
            CO_exit(EXIT_FAILURE);
    }


    if (!asm_gas_fn && !asm_asmpro_fn && !binfn)
        CO_exit(0);

    /*
     * register allocation
     */

    LOG_printf (LOG_INFO, "PASS 2: register allocation\n");
    for (CG_fragList fl=frags; fl; fl=fl->tail)
    {
        CG_frag frag = fl->head;
        if (frag->kind != CG_procFrag)
            continue;

        Temp_label   label   = frag->u.proc.label;
        CG_frame     frame   = frag->u.proc.frame;
        AS_instrList body    = frag->u.proc.body;

        LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "\n************************************************************************************************\n");
        LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "**\n");
        LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "** register allocation for %s\n", Temp_labelstring(label));
        LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "**\n");
        LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "************************************************************************************************\n\n");
        LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, ">>>>>>>>>>>>>>>>>>>>> Proc %s AS stmt list after codegen, before regalloc:\n", Temp_labelstring(label));
        AS_logInstrList (body);
        LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "<<<<<<<<<<<<<<<<<<<<< Proc %s AS stmt list after codegen, before regalloc.\n", Temp_labelstring(label));
        U_memstat();

        if (!LS_regalloc(frame, body) || EM_anyErrors)
        {
            LOG_printf (LOG_ERROR, "\n\nregister allocation failed - exiting.\n");
            CO_exit(EXIT_FAILURE);
        }

        CG_procEntryExitAS(frag);

        LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, ">>>>>>>>>>>>>>>>>>>>> Proc %s AS stmt list (after CG_procEntryExitAS):\n", Temp_labelstring(label));
        AS_logInstrList(body);
        LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "<<<<<<<<<<<<<<<<<<<<< Proc %s AS stmt list (after CG_procEntryExitAS).\n", Temp_labelstring(label));
        U_memstat();
    }

    /*
     * generate target assembly code
     */

    if (asm_gas_fn)
    {
        FILE *out = fopen(asm_gas_fn, "w");
        if (!out)
        {
            LOG_printf (LOG_ERROR, "\n\nfailed to open asm file %s for writing.\n", asm_gas_fn);
            CO_exit(EXIT_FAILURE);
        }
        CG_writeASMFile (out, frags, AS_dialect_gas);
        fclose(out);
        LOG_printf (LOG_INFO, "created GNU style asm file: %s\n", asm_gas_fn);
    }

    if (asm_asmpro_fn)
    {
        FILE *out = fopen(asm_asmpro_fn, "w");
        if (!out)
        {
            LOG_printf (LOG_ERROR, "\n\nfailed to open asm file %s for writing.\n", asm_asmpro_fn);
            CO_exit(EXIT_FAILURE);
        }
        CG_writeASMFile (out, frags, AS_dialect_ASMPro);
        fclose(out);
        LOG_printf (LOG_INFO, "created ASMPro style asm file: %s\n", asm_asmpro_fn);
    }

    if (asm_vasm_fn)
    {
        FILE *out = fopen(asm_vasm_fn, "w");
        if (!out)
        {
            LOG_printf (LOG_ERROR, "\n\nfailed to open asm file %s for writing.\n", asm_vasm_fn);
            CO_exit(EXIT_FAILURE);
        }
        CG_writeASMFile (out, frags, AS_dialect_vasm);
        fclose(out);
        LOG_printf (LOG_INFO, "created vasm style asm file: %s\n", asm_vasm_fn);
    }

    if (!binfn && !objfn)
        CO_exit(0);

    /*
     * machine code generation (assembly phase)
     */

    LOG_printf (LOG_INFO, "PASS 3: assembler\n");
    AS_object obj = AS_Object(binfn, "aqb");

    for (CG_fragList fl=frags; fl; fl=fl->tail)
    {
        CG_frag frag = fl->head;
        switch (frag->kind)
        {
            case CG_procFrag:
            {
                Temp_label   label   = frag->u.proc.label;
                CG_frame     frame   = frag->u.proc.frame;
                AS_instrList body    = frag->u.proc.body;
                bool         expt    = frag->u.proc.expt;

                LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "\n************************************************************************************************\n");
                LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "**\n");
                LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "** machine code generation for %s\n", Temp_labelstring(label));
                LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "**\n");
                LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "************************************************************************************************\n\n");
                U_memstat();

                if (!AS_assembleCode (obj, body, expt, frame))
                    CO_exit(EXIT_FAILURE);

                break;
            }
            case CG_stringFrag:
                AS_assembleDataAlign2 (obj);
                if (!AS_assembleString (obj, frag->u.stringg.label, frag->u.stringg.str, frag->u.stringg.msize))
                    CO_exit(EXIT_FAILURE);
                break;
            case CG_dataFrag:
                if (!frag->u.data.size)
                    break;
                AS_assembleDataAlign2 (obj);
                if (!AS_assembleDataLabel (obj, frag->u.data.label, frag->u.data.expt, frag->u.data.ty))
                    CO_exit(EXIT_FAILURE);
                if (frag->u.data.init)
                {
                    for (CG_dataFragNode n=frag->u.data.init; n; n=n->next)
                    {
                        switch (n->kind)
                        {
                            case CG_labelNode:
                                if (!AS_assembleDataLabel (obj, n->u.l.label, n->u.l.expt, /*ty=*/NULL))
                                    CO_exit(EXIT_FAILURE);
                                break;
                            case CG_constNode:
                            {
                                Ty_const c = n->u.c;
                                switch (c->ty->kind)
                                {
                                    case Ty_bool:
                                    case Ty_byte:
                                    case Ty_ubyte:
                                        // FIXME AS_assembleData8 (obj->dataSeg, c->u.b);
                                        assert(FALSE);
                                        break;
                                    case Ty_uinteger:
                                    case Ty_integer:
                                        AS_assembleData16 (obj->dataSeg, c->u.i);
                                        break;
                                    case Ty_long:
                                    case Ty_ulong:
                                    case Ty_pointer:
                                    case Ty_any:
                                        AS_assembleData32 (obj->dataSeg, c->u.i);
                                        break;
                                    case Ty_single:
                                        AS_assembleData32 (obj->dataSeg, encode_ffp(c->u.f));
                                        break;
                                    case Ty_string:
                                        AS_assembleDataString (obj->dataSeg, c->u.s);
                                        break;
                                    case Ty_sarray:
                                    case Ty_darray:
                                    case Ty_class:
                                    case Ty_interface:
                                    case Ty_record:
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
                            case CG_ptrNode:
                            {
                                if (n->u.ptr)
                                    AS_assembleDataPtr (obj, n->u.ptr);
                                else
                                    AS_assembleData32 (obj->dataSeg, 0);
                                break;
                            }
                        }
                    }
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

    AS_resolveLabels (UP_assem, obj);

    if (EM_anyErrors)
    {
        LOG_printf (LOG_ERROR, "\n\nassembler failed - exiting.\n");
        CO_exit(EXIT_FAILURE);
    }

    if (objfn)
        LI_segmentWriteObjectFile (obj, objfn);

    if (!binfn)
        CO_exit(0);

    /*
     * machine code generation (link phase)
     */

    LOG_printf (LOG_INFO, "PASS 4: linker\n");
    LI_segmentList sl = LI_SegmentList();

    LOG_printf (LOG_INFO, "        reading startup.o\n");
    FILE *fObj = E_openModuleFile ("startup.o");
    if (!fObj)
    {
        LOG_printf (LOG_ERROR, "*** ERROR: failed to open startup.o\n\n");
        CO_exit(EXIT_FAILURE);
    }
    if (!LI_segmentListReadObjectFile (UP_link, sl, "startup.o", fObj))
    {
        fclose(fObj);
        CO_exit(EXIT_FAILURE);
    }
    fclose(fObj);

    if (obj->codeSeg)
        LI_segmentListAppend (sl, obj->codeSeg);
    if (obj->dataSeg)
        LI_segmentListAppend (sl, obj->dataSeg);

    for (E_moduleListNode n = E_getLoadedModuleList(); n; n=n->next)
    {
        if (!n->m->hasCode)
            continue;

        static char mod_fn[PATH_MAX];
        snprintf (mod_fn, PATH_MAX, "%s.a", S_name (n->m->name));
        LOG_printf (LOG_INFO, "        reading %s\n", mod_fn);
        fObj = E_openModuleFile (mod_fn);
        if (!fObj)
        {
            LOG_printf (LOG_ERROR, "*** ERROR: failed to open %s\n\n", mod_fn);
            CO_exit(EXIT_FAILURE);
        }
        if (!LI_segmentListReadObjectFile (UP_link, sl, S_name(n->m->name), fObj))
        {
            fclose(fObj);
            CO_exit(EXIT_FAILURE);
        }
        fclose(fObj);
    }

    if (!LI_link (UP_link, sl))
    {
        LOG_printf (LOG_ERROR, "*** ERROR: failed to link.\n\n");
        CO_exit(EXIT_FAILURE);
    }

    LI_segmentListWriteLoadFile (sl, binfn);

    float endTime = U_getTime();
    LOG_printf (LOG_INFO, "\ncompilation finished, took %ds.\n", (int)(endTime-startTime));

    CO_exit(0);

    return 0;
}


