
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

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;
extern struct GfxBase       *GfxBase;
extern struct IntuitionBase *IntuitionBase;

#define FONT_DIR  "SYS:x/Fonts"
#define FONT_NAME "Dale"
#define FONT_SIZE "8"

struct DiskFontHeader *_loadFont (char *font_dir, char *font_name, char *font_size)
{
    static char fontPath[256];

    strncpy (fontPath, font_dir, 256);
    AddPart ((STRPTR) fontPath, (STRPTR)font_name, 256);
    AddPart ((STRPTR) fontPath, (STRPTR)font_size, 256);

    printf ("fontPath: %s\n", fontPath);

    BPTR seglist = LoadSeg ((STRPTR)fontPath);

    if (seglist)
    {
        printf ("LoadSeg worked: 0x%08lx\n", seglist);

		struct DiskFontHeader *dfh;

		dfh = (struct DiskFontHeader *) (BADDR(seglist) + 8);
        dfh->dfh_Segment = seglist;

		printf ("dfh->dfh_Name: %s, revision: %d\n", dfh->dfh_Name, dfh->dfh_Revision);

		struct TextFont *tf = &dfh->dfh_TF;
		printf ("tf->tf_YSize    =%d\n", tf->tf_YSize    );
		printf ("tf->tf_Style    =%d\n", tf->tf_Style    );
		printf ("tf->tf_Flags    =%d\n", tf->tf_Flags    );
		printf ("tf->tf_XSize    =%d\n", tf->tf_XSize    );
		printf ("tf->tf_Baseline =%d\n", tf->tf_Baseline );
		printf ("tf->tf_BoldSmear=%d\n", tf->tf_BoldSmear);

        return dfh;
    }
    else
    {
        printf ("LoadSeg failed!\n");
    }

    return NULL;
}

void _freeFont (struct DiskFontHeader *dfh)
{
    BPTR seglist = dfh->dfh_Segment;
    if (!seglist)
        return;
    dfh->dfh_Segment = 0l;
    UnLoadSeg (seglist);
}

//typedef UBYTE glyph8_t[8];
//typedef glyph8_t fontData_t[256];

void _fontConv(struct TextFont *font, UBYTE *fontData)
{
    printf ("fontConv... blanking\n"); Delay(100);
    for (UWORD ci=0; ci<256; ci++)
    {
        printf ("   ci=%d\n", ci);
        for (UBYTE y=0; y<8; y++)
            *(fontData + ci*8+y) = 0;
    }

    UWORD *pCharLoc = font->tf_CharLoc;
#ifdef DEBUG_FONTCONV
    uint16_t cnt=0;
#endif
    printf ("fontConv... converting\n"); Delay(100);
    for (UBYTE ci=font->tf_LoChar; ci<font->tf_HiChar; ci++)
    {
        printf ("   ci=%d\n", ci);
        UWORD bl = *pCharLoc;
        UWORD byl = bl / 8;
        BYTE bitl = bl % 8;
        pCharLoc++;
        UWORD bs = *pCharLoc;
        pCharLoc++;
#ifdef DEBUG_FONTCONV
        if (cnt<DEBUG_FONTCONV_NUM)
            printf ("ci=%d(%c) bl=%d -> byl=%d/bitl=%d, bs=%d\n", ci, ci, bl, byl, bitl, bs);
#endif
        if (bs>8)
            bs = 8;
        for (UBYTE y=0; y<font->tf_YSize; y++)
        {
            char *p = font->tf_CharData;
            p += y*font->tf_Modulo + byl;
            BYTE bsc = bs;
            BYTE bitlc = 7-bitl;
            for (BYTE x=7; x>=0; x--)
            {
                if (*p & (1<<bitlc))
                {
                    *(fontData + ci*8+y) |= (1<<x);
#ifdef DEBUG_FONTCONV
                    if (cnt<DEBUG_FONTCONV_NUM)
                        printf("*");
#endif
                }
                else
                {
#ifdef DEBUG_FONTCONV
                    if (cnt<DEBUG_FONTCONV_NUM)
                        printf(".");
#endif
                }
                bsc--;
                if (!bsc)
                    break;
                bitlc--;
                if (bitlc<0)
                {
                    bitlc = 7;
                    p++;
                }
            }
#ifdef DEBUG_FONTCONV
            if (cnt<DEBUG_FONTCONV_NUM)
                printf("\n");
#endif
        }
#ifdef DEBUG_FONTCONV
        cnt++;
#endif
    }
}

int main (int argc, char *argv[])
{
    struct DiskFontHeader *dfh = _loadFont (FONT_DIR, FONT_NAME, FONT_SIZE);

    if (dfh)
    {
        struct TextFont *tf = &dfh->dfh_TF;

        printf ("font loaded. rf=0x%08x\n", (uint32_t)tf);
        Delay(100);

        static UBYTE fontData[256][8];
        _fontConv (tf, (UBYTE *)fontData);

        Delay(300);   /* should be rest_of_program */

        _freeFont(dfh);
    }

    return 0;
}

