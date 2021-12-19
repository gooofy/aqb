
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <libraries/diskfont.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>
#include <inline/graphics.h>
#include <inline/intuition.h>

#include "amigasupport.h"

#define MAKE_ID(a,b,c,d) ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;
extern struct GfxBase       *GfxBase;
extern struct IntuitionBase *IntuitionBase;
struct Library              *MUIMasterBase;

/* Include appropiate headers*/
#include <libraries/mui.h>
#include <proto/muimaster.h>
//#include <inline/muimaster.h>

#define MyMUI_NewObject(___classID, ___tagList, ...) \
    ({_sfdc_vararg _tags[] = { ___tagList, __VA_ARGS__ }; MUI_NewObjectA((___classID), (struct TagItem *) _tags); })


int main (int argc, char *argv[])
{

    printf ("MUI test\n");

    MUIMasterBase = OpenLibrary((STRPTR)"muimaster.library", MUIMASTER_VMIN);
    if (MUIMasterBase)
    {
        printf ("muimaster.library opened. MUIC_Window=%s\n", MUIC_Window);

		Object *app, *win1, *label, *bar, *button;

		app = MyMUI_NewObject( (CONST_STRPTR) MUIC_Application,
			MUIA_Application_Title      , (ULONG) "AppWindowDemo",
			MUIA_Application_Version    , (ULONG) "$VER: AppWindowDemo 21.2 (08.02.2018)",
			MUIA_Application_Copyright  , (ULONG) "(C) 1992-2006 Stefan Stuntz, (C) 2006-2020 Thore Boeckelmann, Jens Maus",
			MUIA_Application_Author     , (ULONG) "Stefan Stuntz, Thore Boeckelmann, Jens Maus",
			MUIA_Application_Description, (ULONG) "Show AppWindow Handling",
			MUIA_Application_Base       , (ULONG) "APPWINDOWDEMO",

			MUIA_Application_Window, (ULONG) (win1 = MyMUI_NewObject((CONST_STRPTR) MUIC_Window,
				MUIA_Window_Title, (ULONG) "Window Title",
				//MUIA_Window_ID   , MAKE_ID('E','M','R','T'),
				WindowContents, (ULONG) MyMUI_NewObject((CONST_STRPTR) MUIC_Group,
					Child, (ULONG) (label  = MUI_MakeObject(MUIO_Label, (ULONG) "I am MUI Application on Amiga 3.X", 0)),
					Child, (ULONG) (bar    = MUI_MakeObject(MUIO_HBar, 4)),
					Child, (ULONG) (button = MUI_MakeObject(MUIO_Button, (ULONG) "Quit")),
				TAG_DONE),
			TAG_DONE)),
		TAG_DONE);

#if 0




        Object *myLabel = MUI_MakeObject (MUIO_Label, (ULONG) "I am MUI Application on Amiga 3.X", 0);
        if (!myLabel)
        {
			printf("Cannot create label.\n");
			return 0;
        }
        printf ("label created: myLabel=0x%08lx\n", (ULONG)myLabel);

        Object *myGroup = MUI_NewObject((CONST_STRPTR) MUIC_Group,
					                    MUIA_Group_Child, myLabel,
					                    MUIA_Group_Child, MUI_MakeObject(MUIO_HBar, 4),
					                    Child, MUI_MakeObject(MUIO_Button, (ULONG) "Quit"),
                                        TAG_DONE);

        if (!myGroup)
        {
			printf("Cannot create group.\n");
			return 0;
        }
        printf ("group created: myGroup=0x%08lx\n", (ULONG)myGroup);

        Object *win1 = MUI_NewObject((CONST_STRPTR) MUIC_Window,
                                     MUIA_Window_Title     , "Window Title",
                                     MUIA_Window_ID        , MAKE_ID('E','M','R','T'),
                                     MUIA_Window_AppWindow , TRUE,
                                     MUIA_Window_RootObject, myGroup,
                                     TAG_DONE);
#endif
        printf ("app=0x%08lx, win1=0x%08lx, label=0x%08lx, bar=0x%08lx, button=0x%08lx\n", (ULONG)app, (ULONG)win1, (ULONG)label, (ULONG) bar, (ULONG) button);
        if (!win1)
        {
			printf("Cannot create win1.\n");
			return 0;
        }
        printf ("win1 created: win1=0x%08lx\n", (ULONG)win1);

        if (!app)
        {
			printf("Cannot create app.\n");
			return 0;
        }
        printf ("app created: app=0x%08lx\n", (ULONG)app);

#if 0
        Object *myGroup3 = MUI_NewObject((STRPTR) "Group.mui",
					                     MUIA_Group_Child, myLabel,
					                     MUIA_Group_Child, MUI_MakeObject(MUIO_HBar, 4, 0),
					                     //Child, closeButton=MUI_MakeObject(MUIO_Button, (ULONG) "Quit", 0),
                                         TAG_DONE);

        if (!myGroup3)
        {
			printf("Cannot create group3.\n");
			return 0;
        }
        printf ("group3 created: myGroup3=0x%08lx\n", (ULONG)myGroup3);

        Object *win1 = MUI_NewObject((STRPTR) "Group.mui",
					                     MUIA_Group_Child, myLabel,
					                     MUIA_Group_Child, MUI_MakeObject(MUIO_HBar, 4, 0),
					                     //Child, closeButton=MUI_MakeObject(MUIO_Button, (ULONG) "Quit", 0),
                                         TAG_DONE);
        //Object *win1 = MUI_NewObject( (STRPTR) "Window23444.mui",
        //                              //MUIA_Window_Title     , "Window Title",
        //                              //MUIA_Window_ID        , MAKE_ID('E','M','R','T'),
        //                              //MUIA_Window_AppWindow , TRUE,
        //                              //MUIA_Window_RootObject, myGroup,
        //                              //WindowContents, myLabel,
        //                              TAG_DONE);

        if (!win1)
        {
			printf("Cannot create window.\n");
			return 0;
        }
        printf ("window created: win1=0x%08lx\n", (ULONG)win1);

		Object *app;

		app = MUI_NewObject( (STRPTR) "Application.mui",
			                 MUIA_Application_Title      , "Project",
			                 MUIA_Application_Version    , "$VER: Project X.X (XX.XX.XX)",
			                 MUIA_Application_Copyright  , " ",
			                 MUIA_Application_Author     , " ",
			                 MUIA_Application_Description, " ",
			                 MUIA_Application_Base       , " ",

			                 MUIA_Application_Window     , win1,
                             TAG_DONE);

		if (!app)
		{
			printf("Cannot create application.\n");
			return 0;
		}

        printf ("application created: app=0x%08lx win1=0x%08lx\n", (ULONG) app, (ULONG)win1);
#endif
        ASUP_DoMethod(win1, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

        //ASUP_DoMethod(closeButton, MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    	set(win1, MUIA_Window_Open, TRUE);// open window

        printf ("window opened.\n");

		BOOL running = TRUE;
		while(running)
		{
            ULONG signals;
            printf ("running...\n");

			ULONG id = ASUP_DoMethod(app,MUIM_Application_Input,&signals);

			switch(id)
			{
					case MUIV_Application_ReturnID_Quit:
						if ((MUI_RequestA(app, 0, 0, (STRPTR)"Quit?", (STRPTR)"_Yes|_No", (STRPTR)"\33cAre you sure?", 0)) == 1)
							running = FALSE;
					break;
			}
			if (running && signals) Wait(signals);
		}

		set(win1,MUIA_Window_Open,FALSE);

		if (app) MUI_DisposeObject(app);

		CloseLibrary (MUIMasterBase);
	}
	else
	{
		printf("Failed to open Mui Master library.\n");
		exit(5);
	}

	return 0;
}

