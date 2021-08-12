
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>
#include <inline/graphics.h>
#include <inline/intuition.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;
extern struct GfxBase       *GfxBase;
extern struct IntuitionBase *IntuitionBase;

#define BM_DEPTH  2
#define BM_WIDTH  320
#define BM_HEIGHT 100

int main (int argc, char *argv[])
{
    struct Screen *my_screen;
    ULONG  screen_modeID;

    struct Screen   *pub_screen = NULL;
    struct DrawInfo *screen_drawinfo = NULL;

    STRPTR pub_screen_name = (STRPTR)"workbench";

    pub_screen = LockPubScreen(pub_screen_name);
    if (pub_screen != NULL)
    {
        screen_drawinfo = GetScreenDrawInfo(pub_screen);
        if (screen_drawinfo != NULL)
        {
            screen_modeID = GetVPModeID(&(pub_screen->ViewPort));
            if( screen_modeID != INVALID_ID )
            {
                my_screen = OpenScreenTags(NULL,
                    SA_Width,      pub_screen->Width,
                    SA_Height,     pub_screen->Height,
                    SA_Depth,      screen_drawinfo->dri_Depth,
                    SA_Overscan,   OSCAN_TEXT,
                    SA_AutoScroll, TRUE,
                    SA_Pens,       (ULONG)(screen_drawinfo->dri_Pens),
                    SA_DisplayID,  screen_modeID,
                    SA_Title,      (ULONG) "Cloned Screen",
                    TAG_END);
                if (my_screen != NULL)
                {
                    FreeScreenDrawInfo(pub_screen,screen_drawinfo);
                    screen_drawinfo = NULL;
                    UnlockPubScreen(pub_screen_name,pub_screen);
                    pub_screen = NULL;

					struct BitMap *renderBitMap;
					WORD planeNum;
					BOOL allocatedBitMaps = FALSE;

					if ((renderBitMap = AllocMem(sizeof(struct BitMap), MEMF_PUBLIC | MEMF_CLEAR)))
					{
						InitBitMap(renderBitMap, BM_DEPTH, BM_WIDTH, BM_HEIGHT);

						allocatedBitMaps = TRUE;
						for (planeNum = 0; (planeNum < BM_DEPTH) && (allocatedBitMaps == TRUE); planeNum++)
						{
							renderBitMap->Planes[planeNum] = AllocRaster(BM_WIDTH, BM_HEIGHT);
							if (NULL == renderBitMap->Planes[planeNum])
								allocatedBitMaps = FALSE;
						}

						for (UWORD y = 0; y<BM_HEIGHT; y++)
						{
							for (UWORD x = 0; x<BM_WIDTH/8; x++)
							{
								UBYTE *ptr = renderBitMap->Planes[0] + y * renderBitMap->BytesPerRow + x;
								*ptr = x > 10 ? 0xff : 0;
								ptr = renderBitMap->Planes[1] + y * renderBitMap->BytesPerRow + x;
								*ptr = 0;
							}
						}

						BltBitMapRastPort (renderBitMap, 0, 0, &my_screen->RastPort, 23, 42, BM_WIDTH, BM_HEIGHT, 0xc0);

						Delay(300);   /* should be rest_of_program */

						for (planeNum = 0; planeNum < BM_DEPTH; planeNum++)
						{
							if (NULL != renderBitMap->Planes[planeNum])
								FreeRaster(renderBitMap->Planes[planeNum], BM_WIDTH, BM_HEIGHT);
						}
						FreeMem(renderBitMap, sizeof(struct BitMap));
					}

                    CloseScreen(my_screen);
                }
            }
        }
    }

    if (screen_drawinfo != NULL )
        FreeScreenDrawInfo(pub_screen,screen_drawinfo);
    if (pub_screen != NULL )
        UnlockPubScreen(pub_screen_name,pub_screen);

    return 0;
}

