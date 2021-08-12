
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
#include <clib/diskfont_protos.h>
#include <clib/graphics_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>
#include <inline/graphics.h>
#include <inline/diskfont.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;
extern struct GfxBase       *GfxBase;
extern struct DiskfontBase  *DiskfontBase;

static void dumpFont(char *fontName, uint16_t fontSize, FILE *fSrc)
{
    struct TextAttr attr = { (STRPTR)fontName, fontSize, 0, 0 };

    struct TextFont *font = OpenDiskFont(&attr);
    if (!font)
    {
        printf("Can't open font %s\n", fontName);
        return;
    }

    static UBYTE fontData[256][8];
    for (UWORD ci=0; ci<256; ci++)
        for (UBYTE y=0; y<8; y++)
            fontData[ci][y] = 0;

    UWORD *pCharLoc = font->tf_CharLoc;
#ifdef DEBUG_FONTCONV
    uint16_t cnt=0;
#endif
    for (UBYTE ci=font->tf_LoChar; ci<font->tf_HiChar; ci++)
    {
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
                    fontData[ci][y] |= (1<<x);
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
    CloseFont(font);

    fprintf (fSrc, "    {\n");
    fprintf (fSrc, "        {");
    BOOL first = TRUE;
    for (UWORD ci=0; ci<256; ci++)
    {
        for (UBYTE y=0; y<8; y++)
        {
            if (first)
                first = FALSE;
            else
                fprintf (fSrc, ", ");
            fprintf (fSrc, "0x%02x", fontData[ci][y]);
        }
        if (ci<255)
            fprintf (fSrc, "},\n        {");
        else
            fprintf (fSrc, "}\n");
        first = TRUE;
    }
    fprintf (fSrc, "    }");
}

int main (int argc, char *argv[])
{
    FILE *fSrc;

    fSrc = fopen ("fonts.h", "w");
    fprintf (fSrc, "static UBYTE g_fontData[2][256][8] = {\n");
    dumpFont ("aqb.font", 6, fSrc);
    fprintf (fSrc, ",\n");
    dumpFont ("aqb.font", 8, fSrc);
    fprintf (fSrc, "\n};\n");
    fclose (fSrc);

    printf ("fonts.h written.\n");

    return 0;
}

