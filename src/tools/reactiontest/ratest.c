#include <stdlib.h>
#include <intuition/classusr.h>
#include <gadgets/layout.h>
#include <classes/window.h>
#define __CLIB_PRAGMA_LIBCALL
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/layout.h>
#include <proto/window.h>
#include <proto/intuition.h>

struct IntuitionBase *IntuitionBase;
struct Library *WindowBase;
struct Library *LayoutBase;

void cleanexit(Object *windowObject);
void processEvents(Object *windowObject);

int main(void)
{
    struct Window *intuiwin = NULL;
    Object *windowObject = NULL;
    Object *mainLayout = NULL;

    if (! (IntuitionBase = (struct IntuitionBase*)OpenLibrary((STRPTR)"intuition.library",47)))
        cleanexit(NULL);
    if (! (WindowBase = OpenLibrary((STRPTR)"window.class", 47)))
        cleanexit(NULL);
    if (! (LayoutBase = OpenLibrary((STRPTR)"gadgets/layout.gadget", 47)))
        cleanexit(NULL);

    mainLayout = NewObject(LAYOUT_GetClass(), NULL,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_DeferLayout, TRUE,
        LAYOUT_SpaceInner, TRUE,
        LAYOUT_SpaceOuter, TRUE,
        TAG_DONE);

    windowObject = NewObject(WINDOW_GetClass(), NULL,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WA_Activate, TRUE,
        WA_Title, (ULONG)"BOOPSI window demo",
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
        cleanexit(NULL);
    if (! (intuiwin = (struct Window *) DoMethod(windowObject,WM_OPEN, NULL)))
        cleanexit(windowObject);
    processEvents(windowObject);
    DoMethod(windowObject,WM_CLOSE);
    cleanexit(windowObject);
}
void processEvents(Object *windowObject)
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

void cleanexit(Object *windowObject)
{
    if (windowObject)
        DisposeObject(windowObject);
    CloseLibrary((struct Library*)IntuitionBase);
    CloseLibrary(WindowBase);
    CloseLibrary(LayoutBase);
    exit(0);
}

