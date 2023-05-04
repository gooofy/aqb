#define ENABLE_DPRINTF

#include "../_aqb/_aqb.h"
#include "../_brt/_brt.h"

#include "IntuiSupport.h"

#include <exec/types.h>
//#include <exec/memory.h>
//#include <clib/exec_protos.h>
//#include <inline/exec.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <clib/intuition_protos.h>
#include <inline/intuition.h>

#include <clib/graphics_protos.h>
#include <inline/graphics.h>

intuis_win_ext_t    _g_intuis_win_ext[MAX_NUM_WINDOWS];

static void _IntuiSupport_shutdown(void)
{
    DPRINTF ("_IntuiSupport_shutdown called\n");

    for (int i=0; i<MAX_NUM_WINDOWS; i++)
    {
        intuis_win_ext_t *ext = &_g_intuis_win_ext[i];
        DPRINTF ("_IntuiSupport_shutdown window #%d, ext=0x%08lx\n", i+1, ext);
        if (ext->draw_info)
        {
            DPRINTF ("_IntuiSupport_shutdown window #%d FreeScreenDrawInfo()\n", i+1);
            FreeScreenDrawInfo (ext->screen, ext->draw_info);
        }
    }

    DPRINTF ("_IntuiSupport_shutdown done\n");
}

static void window_close_cb (short win_id, void *ud)
{
    DPRINTF ("IntuiSupport: window_close_cb called on win #%d\n", win_id);

    // FIXME:
    //intuis_win_ext_t *ext = &_g_intuis_win_ext[win_id-1];
    //struct Window *win = _aqb_get_win(win_id);
    //_gtgadgets_free (win, ext);
}

intuis_win_ext_t * _IntuiSupport_get_ext(short win_id)
{
    DPRINTF ("_IntuiSupport_get_ext win_id=%d\n", win_id);
    intuis_win_ext_t *ext = &_g_intuis_win_ext[win_id-1];
    DPRINTF ("_IntuiSupport_get_ext ext=0x%08lx\n", ext);

    if (!ext->screen)
    {
        _aqb_get_output (/*needGfx=*/TRUE);
        struct Window *win = _aqb_get_win(win_id);
        if (!win)
            ERROR (AE_MENU_NO_WINDOW);
        struct Screen *scr = win->WScreen;
        ext->screen = scr;
        ext->screen_font = OpenFont(scr->Font);
        ext->draw_info   = GetScreenDrawInfo(scr);
    }

    if (!ext->close_cb_installed)
    {
        DPRINTF ("_IntuiSupport_get_ext: installing custom close callback, win_id=%d\n", win_id);
        _window_add_close_cb (win_id, window_close_cb, NULL);
        ext->close_cb_installed = TRUE;
    }

    return ext;
}

void _IntuiSupport_init(void)
{
    for (int i=0; i<MAX_NUM_WINDOWS; i++)
    {
        intuis_win_ext_t *ext = &_g_intuis_win_ext[i];

        ext->screen             = NULL;
        ext->screen_font        = NULL;
        ext->draw_info          = NULL;
        ext->deployedMenu       = NULL;
        ext->close_cb_installed = FALSE;
    }

    ON_EXIT_CALL(_IntuiSupport_shutdown);
}

