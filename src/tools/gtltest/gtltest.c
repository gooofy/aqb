#define ENABLE_DPRINTF

#include "minrt.h"

#include <stdlib.h>

#include <proto/alib.h>

#include <proto/exec.h>
#include <inline/exec.h>

#include <dos/dos.h>
#include <inline/dos.h>

#include <proto/intuition.h>
#include <inline/intuition.h>

#include <workbench/workbench.h>

#include <inline/gadtools.h>

#include "include/libraries/gtlayout.h"
#include "include/proto/gtlayout_protos.h"
#include "include/inline/gtlayout.h"


ULONG _aqb_stack_size = 0;

extern struct WBStartup     *_WBenchMsg;
struct Library              *GTLayoutBase  = NULL;
struct Library              *GadToolsBase  = NULL;

static void exit_cb(void)
{
    if (GTLayoutBase)
        CloseLibrary(GTLayoutBase);
    if (GadToolsBase)
        CloseLibrary(GadToolsBase);
}

//static void processEvents(Object *windowObject)
//{
//    ULONG windowsignal;
//    //ULONG receivedsignal;
//    ULONG result;
//    ULONG code;
//    BOOL end = FALSE;
//    GetAttr(WINDOW_SigMask, windowObject, &windowsignal);
//    while (!end)
//    {
//        /*receivedsignal =*/ Wait(windowsignal);
//        while ((result = DoMethod(windowObject, WM_HANDLEINPUT, &code)) != WMHI_LASTMSG)
//        {
//            switch (result & WMHI_CLASSMASK)
//            {
//                case WMHI_CLOSEWINDOW:
//                    end=TRUE;
//                    break;
//            }
//        }
//    }
//}

int main(void)
{
    atexit(exit_cb);

    if (! (GadToolsBase = OpenLibrary((STRPTR)"gadtools.library", 39)))
        _exit_msg(20, "failed to open gadtools.library\n");
    DPRINTF ("gadtools.library opened\n");

    if (! (GTLayoutBase = OpenLibrary((STRPTR)"gtlayout.library", 39)))
        _exit_msg(20, "failed to open gtlayout.library\n");
    DPRINTF ("gtlayout.library opened\n");

	struct LayoutHandle *Handle;

	if ((Handle = LT_CreateHandleTags(NULL, LAHN_AutoActivate,FALSE, TAG_DONE)))
	{
       DPRINTF ("LT_CreateHandleTags worked.\n");
	   struct Window *Window;

	   LT_New(Handle,
	             LA_Type,      VERTICAL_KIND,  /* A vertical group. */
	             LA_LabelText, (ULONG)"Main group",   /* Group title text. */
	          TAG_DONE);

	   {
	      LT_New(Handle,
	         LA_Type,      BUTTON_KIND, /* A plain button. */
	         LA_LabelText, (ULONG)"A button",
	         LA_ID,        11,
	      TAG_DONE);

	      LT_New(Handle,
	         LA_Type,      XBAR_KIND,   /* A separator bar. */
	      TAG_DONE);

	      LT_New(Handle,
	         LA_Type,      BUTTON_KIND, /* A plain button. */
	         LA_LabelText, (ULONG)"Another button",
	         LA_ID,        22,
	      TAG_DONE);

	      LT_New(Handle,
	         LA_Type,      END_KIND,    /* This ends the current group. */
	      TAG_DONE);
	   }

	   if ((Window = LT_Build(Handle,
	                          LAWN_Title,     (ULONG)"Window title",
	                          LAWN_IDCMP,     IDCMP_CLOSEWINDOW,
	                          WA_CloseGadget, TRUE,
	                          TAG_DONE)))
	   {
           DPRINTF ("LT_Build worked.\n");
           Delay(50);
#if 1
	       struct IntuiMessage *Message;
	       ULONG                MsgQualifier,
	                            MsgClass;
	       UWORD                MsgCode;
	       struct Gadget       *MsgGadget;
	       BOOL                 Done = FALSE;

	       do
	       {
	           WaitPort(Window->UserPort);

	           while(Message = GT_GetIMsg(Window->UserPort))
	           {
	              MsgClass     = Message->Class;
	              MsgCode      = Message->Code;
	              MsgQualifier = Message->Qualifier;
	              MsgGadget    = Message->IAddress;

	              GT_ReplyIMsg(Message);

	              LT_HandleInput(Handle,MsgQualifier,&MsgClass,
	                  &MsgCode,&MsgGadget);

	              switch(MsgClass)
	              {
	                 case IDCMP_CLOSEWINDOW:

	                     Done = TRUE;
	                     break;

	                 case IDCMP_GADGETUP:

	                     switch(MsgGadget->GadgetID)
	                     {
	                         case 11: printf("First gadget\n");
	                                  break;

	                         case 22: printf("Second gadget\n");
	                                  break;
	                     }

	                     break;
	              }
	           }
	       }
	       while(!Done);
#endif
	   }
       else
       {
           DPRINTF ("LT_Build failed.\n");
       }

	   LT_DeleteHandle(Handle);
	}

    printf("all done. goodbye.\n");

    return 0;
}

