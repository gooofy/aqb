#include "ratest.h"

#include <stdlib.h>

#include <proto/alib.h>

#include <proto/exec.h>
#include <inline/exec.h>

#include <dos/dos.h>
#include <inline/dos.h>

#include <proto/intuition.h>
#include <inline/intuition.h>
#include <intuition/classusr.h>

#include <workbench/workbench.h>

#include <gadgets/button.h>
#include <proto/button.h>
#include <gadgets/layout.h>
#include <proto/layout.h>
#include <classes/window.h>
#include <proto/window.h>

int __nocommandline=1; /* Disable commandline parsing  */
int __initlibraries=0; /* Disable auto-library-opening */

extern struct WBStartup *_WBenchMsg;

struct DosLibrary    *DOSBase       = NULL;
struct IntuitionBase *IntuitionBase = NULL;
struct Library       *WindowBase    = NULL;
struct Library       *LayoutBase    = NULL;
struct Library       *ButtonBase    = NULL;

static BPTR           astdout       = 0;
static Object        *windowObject  = NULL;

void _debug_putc (char c)
{
    if (!astdout)
        return;

    char s[2] = {c, 0};

    Write (astdout, s, 1);
}

static void cleanexit(char *msg)
{
    if (msg)
        DPRINTF (msg);
    if (windowObject)
        DisposeObject(windowObject);

    if (DOSBase)
        CloseLibrary((struct Library*)DOSBase);
    if (IntuitionBase)
        CloseLibrary((struct Library*)IntuitionBase);
    if (WindowBase)
        CloseLibrary(WindowBase);
    if (LayoutBase)
        CloseLibrary(LayoutBase);
    if (ButtonBase)
        CloseLibrary(ButtonBase);

    exit(0);
}

static void processEvents(Object *windowObject)
{
    ULONG windowsignal;
    //ULONG receivedsignal;
    ULONG result;
    ULONG code;
    BOOL end = FALSE;
    GetAttr(WINDOW_SigMask, windowObject, &windowsignal);
    while (!end)
    {
        /*receivedsignal =*/ Wait(windowsignal);
        while ((result = DoMethod(windowObject, WM_HANDLEINPUT, &code)) != WMHI_LASTMSG)
        {
            switch (result & WMHI_CLASSMASK)
            {
                case WMHI_CLOSEWINDOW:
                    end=TRUE;
                    break;
            }
        }
    }
}

int main(void)
{
    //if(_WBenchMsg==NULL)
    //{

    if ( !(DOSBase=(struct DosLibrary *)OpenLibrary((STRPTR)"dos.library", 37)) )
        cleanexit ("failed to open dos.library!\n");
    astdout = Output();
    DPRINTF ("dos.library opened.\n");

    if (! (IntuitionBase = (struct IntuitionBase*)OpenLibrary((STRPTR)"intuition.library", 37)))
        cleanexit("failed to open intuition.library\n");
    DPRINTF ("intuition.library opened\n");

    if (! (WindowBase = OpenLibrary((STRPTR)"window.class", 37)) )
        cleanexit("failed to open window.class\n");
    DPRINTF ("window.class opened\n");

    if (! (LayoutBase = OpenLibrary((STRPTR)"gadgets/layout.gadget", 37)) )
        cleanexit("failed to open gadgets/layout.gadget\n");
    DPRINTF ("gadgets/layout.gadget opened\n");

    if (! (ButtonBase = OpenLibrary((STRPTR)"gadgets/button.gadget", 0)) )
        cleanexit("failed to open gadgets/button.gadget\n");
    DPRINTF ("gadgets/button.gadget opened\n");


    Object *mainLayout = NewObject(LAYOUT_GetClass(), NULL,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_DeferLayout, TRUE,
        LAYOUT_SpaceInner, TRUE,
        LAYOUT_SpaceOuter, TRUE,
        LAYOUT_AddChild, (ULONG) NewObject(BUTTON_GetClass(), NULL, GA_Text, (ULONG) "My first button", TAG_END),
        LAYOUT_AddChild, (ULONG) NewObject(BUTTON_GetClass(), NULL, GA_Text, (ULONG) "My second button", TAG_END),
        TAG_DONE);

    windowObject = NewObject(WINDOW_GetClass(), NULL,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WA_Activate, TRUE,
        WA_Title, (ULONG)"reaction test window",
        WA_DragBar, TRUE,
        WA_CloseGadget, TRUE,
        WA_DepthGadget, TRUE,
        WA_SizeGadget, TRUE,
        WA_InnerWidth, 300,
        WA_InnerHeight, 150,
        WA_IDCMP, IDCMP_CLOSEWINDOW,
        WINDOW_Layout, (ULONG)mainLayout,
        TAG_DONE);

    if (! windowObject)
        cleanexit("failed to create window object\n");

    DPRINTF ("windowObject created.\n");

    struct Window *intuiwin = (struct Window *) DoMethod(windowObject,WM_OPEN, NULL);
    if (!intuiwin)
        cleanexit("failed to open intuition window\n");

    DPRINTF ("intuition window opened.\n");

    //Delay(50);

    processEvents(windowObject);
    DoMethod(windowObject,WM_CLOSE);

    cleanexit("all done. goodbye.\n");

    return 0;
}

