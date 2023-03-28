#!/usr/bin/env python3

#PROPS = { 'name' : 'GTTEXT',
#          'kind' : 'TEXT_KIND',
#          'idcmp': 'TEXTIDCMP',
#                     # name                , OS, type     , getter , default    
#          'props': [ ( 'GTTX_Text'         , 36, 'STRING' , True   , None          ),
#                     ( 'GTTX_CopyText'     , 37, 'BOOLEAN', False  , 'FALSE'       ),
#                     ( 'GTTX_Border'       , 36, 'BOOLEAN', False  , 'TRUE'        ),
#                     ( 'GTTX_FrontPen'     , 39, 'UBYTE',   False  , '1'           ),
#                     ( 'GTTX_BackPen'      , 39, 'UBYTE',   False  , '0'           ),
#                     ( 'GTTX_Justification', 39, 'UBYTE'  , False  , 'GTJ_LEFT'    ),
#                     ( 'GTTX_Clipped'      , 39, 'BOOLEAN', False  , 'TRUE'        )
#                 ]
#        }
#PROPS = { 'name' : 'GTSCROLLER',
#          'kind' : 'SCROLLER_KIND',
#          'idcmp': 'SCROLLERIDCMP',
#                     # name                , OS, type      , getter , default    
#          'props': [
#                     ( 'GA_Disabled'       , 36, 'BOOLEAN' , True   , 'FALSE'       ),
#                     ( 'GA_RelVerify'      , 36, 'BOOLEAN' , False  , 'FALSE'       ),
#                     ( 'GA_Immediate'      , 36, 'BOOLEAN' , False  , 'FALSE'       ),
#                     ( 'GTSC_Top'          , 36, 'INTEGER' , True   , None          ),
#                     ( 'GTSC_Total'        , 36, 'INTEGER' , True   , None          ),
#                     ( 'GTSC_Visible'      , 36, 'INTEGER' , True   , None          ),
#                     ( 'GTSC_Arrows'       , 36, 'UINTEGER', False  , 18            ),
#                     ( 'PGA_Freedom'       , 36, 'ULONG'   , False  , None          ),
#                 ]
#        }
#PROPS = { 'name' : 'GTSTRING',
#          'kind' : 'STRING_KIND',
#          'idcmp': 'STRINGIDCMP',
#          'label': True,
#                     # name                   , OS, type      , getter , default
#          'props': [
#                     ( 'GA_Disabled'          , 36, 'BOOLEAN' , True    , 'FALSE'          ),
#                     ( 'GA_Immediate'         , 36, 'BOOLEAN' , False   , 'FALSE'          ),
#                     ( 'GA_TabCycle'          , 37, 'BOOLEAN' , False   , 'TRUE'           ),
#                     ( 'GTST_String'          , 36, 'STRING'  , False   , 'NULL'           ),
#                     ( 'GTST_MaxChars'        , 36, 'UINTEGER', False   , '256'            ),
#                     ( 'STRINGA_ExitHelp'     , 37, 'BOOLEAN' , False   , 'FALSE'          ),
#                     ( 'STRINGA_Justification', 37, 'STRING'  , False   , 'GACT_STRINGLEFT'),
#                     ( 'STRINGA_ReplaceMode'  , 37, 'BOOLEAN' , False   , 'FALSE'          )
#                 ]
#        }

#PROPS = { 'name' : 'GTINTEGER',
#          'kind' : 'INTEGER_KIND',
#          'idcmp': 'INTEGERIDCMP',
#          'label': True,
#                     # name                   , OS, type      , getter , default
#          'props': [
#                     ( 'GA_Disabled'          , 36, 'BOOLEAN' , True    , 'FALSE'          ),
#                     ( 'GA_Immediate'         , 39, 'BOOLEAN' , False   , 'FALSE'          ),
#                     ( 'GA_TabCycle'          , 37, 'BOOLEAN' , False   , 'TRUE'           ),
#                     ( 'GTIN_Number'          , 36, 'LONG'    , True    , '0'              ),
#                     ( 'GTIN_MaxChars'        , 36, 'UINTEGER', False   , '10'             ),
#                     ( 'STRINGA_ExitHelp'     , 37, 'BOOLEAN' , False   , 'FALSE'          ),
#                     ( 'STRINGA_Justification', 37, 'STRING'  , False   , 'GACT_STRINGLEFT'),
#                     ( 'STRINGA_ReplaceMode'  , 37, 'BOOLEAN' , False   , 'FALSE'          )
#                 ]
#        }

#PROPS = { 'name' : 'GTNUMBER',
#          'kind' : 'NUMBER_KIND',
#          'idcmp': 'NUMBERIDCMP',
#          'label': True,
#                     # name                , OS, type     , getter , default
#          'props': [ ( 'GTNM_Number'       , 36, 'LONG'   , True   , None          ),
#                     ( 'GTNM_Border'       , 36, 'BOOLEAN', False  , 'TRUE'        ),
#                     ( 'GTNM_FrontPen'     , 39, 'UBYTE',   False  , '1'           ),
#                     ( 'GTNM_BackPen'      , 39, 'UBYTE',   False  , '0'           ),
#                     ( 'GTNM_Justification', 39, 'UBYTE'  , False  , 'GTJ_LEFT'    ),
#                     ( 'GTNM_Format'       , 39, 'STRING' , False  , '%ld'         ),
#                     ( 'GTNM_MaxNumberLen' , 39, 'ULONG'  , False  , '10'          ),
#                     ( 'GTNM_Clipped'      , 39, 'BOOLEAN', False  , 'TRUE'        )
#                 ]
#        }

PROPS = { 'name' : 'GTMX',
          'kind' : 'MX_KIND',
          'idcmp': 'MXIDCMP',
          'label': True,
                     # name                , OS, type        , getter , default
          'props': [
                     ( 'GA_Disabled'       , 36, 'BOOLEAN'   , True   , 'FALSE'         ),
                     ( 'GTMX_Labels'       , 36, 'STRING PTR', False  , None            ),
                     ( 'GTMX_Active'       , 36, 'UINTEGER'  , True   , '0'             ),
                     ( 'GTMX_Spacing'      , 36, 'UINTEGER'  , False  , '1'             ),
                     ( 'GTMX_Scaled'       , 39, 'BOOLEAN'   , False  , 'FALSE'         ),
                     ( 'GTMX_TitlePlace'   , 39, 'ULONG'     , False  , 'PLACETEXT_LEFT'),
                 ]
        }

def genline (f, line):
    print (line)
    f.write ("%s\n" % line)

def propname (pn):

    pn2 = pn[pn.index('_')+1:]
    return pn2[0].lower() + pn2[1:]

def gen_basic (f, props):

    genline (f, 'TYPE %s EXTENDS GTGADGET' % props['name'])
    genline (f, '')
    genline (f, '    PUBLIC:')
    cpars = ""
    if props['label']:
        cpars = 'BYVAL label AS STRING, '
    for pn, vers, t, get, default in props['props']:
        if default:
            continue
        cpars = cpars + 'BYVAL %s AS %s, ' % (propname(pn), t)
    genline (f, '        DECLARE CONSTRUCTOR ( %s _' % cpars)
    genline (f, '                              _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER), _')
    genline (f, '                              BYVAL user_data AS VOID PTR=NULL, BYVAL flags AS ULONG=0, BYVAL underscore AS ULONG=95)')
    genline (f, '')
    for pn, vers, t, get, default in props['props']:
        genline (f, '        DECLARE PROPERTY %s AS %s' % (propname(pn), t))
        genline (f, '        DECLARE PROPERTY %s (BYVAL value AS %s)' % (propname(pn), t))
        genline (f, '')
    genline (f, '    PRIVATE:')
    genline (f, '')
    for pn, vers, t, get, default in props['props']:
        genline (f, '        AS %-12s _%s' % (t, propname(pn)))
    genline (f, '')
    genline (f, 'END TYPE')

BT2CT = {'STRING': 'CONST_STRPTR',
         'STRING PTR' : 'CONST_STRPTR *',
         'BOOLEAN' : 'BOOL',
         'UBYTE': 'UBYTE',
         'BYTE': 'BYTE',
         'INTEGER': 'SHORT',
         'UINTEGER': 'USHORT',
         'LONG': 'LONG',
         'ULONG': 'ULONG' }

# basic type to c type
def ct (t):
    return BT2CT[t]

def gen_h (f, props):

    genline (f, '/***********************************************************************************')
    genline (f, ' *')
    genline (f, ' * %s' % props['name'])
    genline (f, ' *')
    genline (f, ' ***********************************************************************************/')
    genline (f, '')
    genline (f, 'typedef struct %s_ %s_t;' % (props['name'], props['name']))
    genline (f, '')
    genline (f, 'struct %s_' % props['name'])
    genline (f, '{')
    genline (f, '    GTGADGET_t      gadget;')
    for pn, vers, t, get, default in props['props']:
        genline (f, '    %-15s %s;' % (ct(t), propname(pn)))
    genline (f, '};')
    genline (f, '')
    genline (f, 'void _%s_CONSTRUCTOR (%s_t *this,' % (props['name'], props['name']))
    cpars = ""
    if props['label']:
        cpars = 'CONST_STRPTR label, '
    for pn, vers, t, get, default in props['props']:
        if default:
            continue
        cpars = cpars + '%s %s, ' % (ct(t), propname(pn))
    genline (f, '                            %s' % cpars)
    genline (f, '                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,')
    genline (f, '                            void *user_data, ULONG flags, ULONG underscore);')
    genline (f, '')
    for pn, vers, t, get, default in props['props']:
        genline (f, '%-15s        _%s_%s_ (%s_t *this);' % (ct(t), props['name'], propname(pn), props['name']))
        genline (f, '%-15s        _%s_%s  (%s_t *this, %s value);' % ('void', props['name'], propname(pn), props['name'], ct(t)))
        # genline (f, '        DECLARE PROPERTY %s (BYVAL value AS %s)' % (propname(pn), t))
        genline (f, '')

def gen_c (f, props):

    genline (f, '#include "../_aqb/_aqb.h"')
    genline (f, '#include "../_brt/_brt.h"')
    genline (f, '')
    genline (f, '#include "GadToolsSupport.h"')
    genline (f, '')
    genline (f, '#include <exec/types.h>')
    genline (f, '#include <exec/memory.h>')
    genline (f, '#include <clib/exec_protos.h>')
    genline (f, '#include <inline/exec.h>')
    genline (f, '')
    genline (f, '#include <intuition/intuition.h>')
    genline (f, '#include <intuition/intuitionbase.h>')
    genline (f, '#include <clib/intuition_protos.h>')
    genline (f, '#include <inline/intuition.h>')
    genline (f, '')
    genline (f, '#include <clib/graphics_protos.h>')
    genline (f, '#include <inline/graphics.h>')
    genline (f, '')
    genline (f, '#include <clib/gadtools_protos.h>')
    genline (f, '#include <inline/gadtools.h>')
    genline (f, '')
    genline (f, 'extern struct Library    *GadToolsBase ;')
    genline (f, '')
    genline (f, 'static struct Gadget *_%s_deploy_cb (GTGADGET_t *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)' % PROPS['name'].lower())
    genline (f, '{')
    genline (f, '    %s_t *gt = (%s_t *)gtg;' % (PROPS['name'], PROPS['name']))
    genline (f, '')
    genline (f, '    gtg->ng.ng_VisualInfo = vinfo;')
    genline (f, '    gtg->ng.ng_TextAttr   = ta;')
    genline (f, '')
    genline (f, '    gtg->gad = CreateGadget (%s, gad, &gtg->ng,' % PROPS['kind'])
    genline (f, '                             GT_Underscore   , gtg->underscore,')
    for pn, vers, t, get, default in props['props']:
        if t != 'STRING':
            genline (f, '                             %-15s , gt->%s,' % (pn, propname(pn)))
        else:
            genline (f, '                             %-15s , (intptr_t) gt->%s,' % (pn, propname(pn)))
    genline (f, '                             TAG_DONE);')
    genline (f, '')
    genline (f, '    if (!gtg->gad)')
    genline (f, '    {')
    genline (f, '        DPRINTF ("_%s_deploy_cb: CreateGadget() failed.\\n");' % PROPS['name'].lower())
    genline (f, '        ERROR(AE_GTG_CREATE);')
    genline (f, '        return gad;')
    genline (f, '    }')
    genline (f, '')
    genline (f, '    // take care of IDCMP flags')
    genline (f, '    ULONG gidcmp = %s;' % PROPS['idcmp'])
    genline (f, '')
    genline (f, '    DPRINTF("_%s_deploy_cb: gtg->win->IDCMPFlags=0x%%08lx, gidcmp=0x%%08lx\\n", gtg->win->IDCMPFlags, gidcmp);' % PROPS['name'].lower())
    genline (f, '')
    genline (f, '    if (gidcmp && ( (gtg->win->IDCMPFlags & gidcmp) != gidcmp ) )')
    genline (f, '        ModifyIDCMP (gtg->win, gtg->win->IDCMPFlags | gidcmp);')
    genline (f, '')
    genline (f, '    return gtg->gad;')
    genline (f, '}')
    genline (f, '')
    genline (f, 'void _%s_CONSTRUCTOR (%s_t *this,' % (props['name'], props['name']))
    cpars = ""
    if props['label']:
        cpars = 'CONST_STRPTR label, '
    for pn, vers, t, get, default in props['props']:
        if default:
            continue
        cpars = cpars + '%s %s, ' % (ct(t), propname(pn))
    genline (f, '                            %s' % cpars)
    genline (f, '                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,')
    genline (f, '                            void *user_data, ULONG flags, ULONG underscore)')
    genline (f, '{')
    genline (f, '    DPRINTF("_%s_CONSTRUCTOR: this=0x%%08lx, x1=%%d, y1=%%d, x2=%%d, y2=%%d\\n", this, x1, y1, x2, y2);' % props['name'])
    genline (f, '    _GTGADGET_CONSTRUCTOR (&this->gadget, %s, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);' % "label" if props['label'] else "")
    genline (f, '    this->gadget.deploy_cb = _%s_deploy_cb;' % (props['name'].lower()))
    for pn, vers, t, get, default in props['props']:
        if not default:
            continue
        genline (f, '    this->%-15s = %s;' % (propname(pn), default))
    genline (f, '}')

    for pn, vers, t, get, default in props['props']:
        genline (f, '')
        genline (f, '%s _%s_%s_ (%s_t *this)' % (ct(t), props['name'], propname(pn), props['name']))
        genline (f, '{')
        if get:
            genline (f, '    if (_GTGADGET_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=%d))' % vers)
            genline (f, '    {')
            genline (f, '        ULONG u;')
            genline (f, '        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, %s, (intptr_t)&u, TAG_DONE);' % pn)
            genline (f, '        if (n==1)')
            if t != 'STRING':
                genline (f, '            return u;')
            else:
                genline (f, '            return (CONST_STRPTR) (intptr_t) u;')
            genline (f, '    }')
        genline (f, '    return this->%s;' % propname(pn))
        genline (f, '}')
        genline (f, 'void _%s_%s (%s_t *this, %s %s)' % (props['name'], propname(pn), props['name'], ct(t), propname(pn)))
        genline (f, '{')
        genline (f, '    if (_GTGADGET_deployed_ (&this->gadget))')
        genline (f, '    {')
        if t != 'STRING':
            genline (f, '        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, %s, %s, TAG_DONE);' % (pn, propname(pn)))
        else:
            genline (f, '        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, %s, (intptr_t) %s, TAG_DONE);' % (pn, propname(pn)))
        genline (f, '    }')
        genline (f, '    this->%s = %s;' % (propname(pn), propname(pn)))
        genline (f, '}')

with open ('foo.bas', 'w') as f:
    gen_basic (f, PROPS)

with open ('foo.h', 'w') as f:
    gen_h (f, PROPS)

with open ('%s.c' % PROPS['name'].lower(), 'w') as f:
    gen_c (f, PROPS)

