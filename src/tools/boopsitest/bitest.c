#include <exec/types.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h> /* contains IDs for gadget attributes */
#include <intuition/icclass.h>     /* contains ICA_MAP, ICA_TARGET       */
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>

struct Library *IntuitionBase;
struct Window *w;
struct IntuiMessage *msg;

/* The attribute mapping lists */
struct TagItem prop2intmap[] =    /* This tells the prop gadget to */
{                                 /* map its PGA_Top attribute to  */
    {PGA_Top,   STRINGA_LongVal}, /* STRINGA_LongVal when it       */
    {TAG_END,}                    /* issues an update about the    */
};                                /* change to its PGA_Top value.  */

struct TagItem int2propmap[] =    /* This tells the string gadget */
{                                 /* to map its STRINGA_LongVal   */
    {STRINGA_LongVal,   PGA_Top}, /* attribute to PGA_Top when it */
    {TAG_END,}                    /* issues an update.            */
};

#define PROPGADGET_ID       1L
#define INTGADGET_ID        2L
#define PROPGADGETWIDTH     10L
#define PROPGADGETHEIGHT    80L
#define INTGADGETHEIGHT     18L
#define VISIBLE             10L
#define TOTAL               100L
#define INITIALVAL          25L
#define MINWINDOWWIDTH      80
#define MINWINDOWHEIGHT     (PROPGADGETHEIGHT + 70)
#define MAXCHARS            3L


int main(void)
{
    BOOL done = FALSE;

    if ( (IntuitionBase = OpenLibrary((STRPTR)"intuition.library", 37L)) )
    {                                /* Open the window--notice that the window's IDCMP port     */
                                     /* does not listen for GADGETUP messages.                   */
        if ( (w = OpenWindowTags(NULL,
                                 WA_Flags,       WFLG_DEPTHGADGET | WFLG_DRAGBAR |
                                                 WFLG_CLOSEGADGET | WFLG_SIZEGADGET,
                                 WA_IDCMP,       IDCMP_CLOSEWINDOW,
                                 WA_MinWidth,    MINWINDOWWIDTH,
                                 WA_MinHeight,   MINWINDOWHEIGHT,
                                 TAG_END)) )
        {
            struct Gadget *prop=NULL, *integer=NULL;
            prop = (struct Gadget *)NewObject(NULL, (STRPTR)"propgclass",
                                              GA_ID,     PROPGADGET_ID,        /* These are defined by gadgetclass and         */
                                              GA_Top,    (w->BorderTop) + 5L,  /* correspond to similarly named fields in      */
                                              GA_Left,   (w->BorderLeft) + 5L, /* the Gadget structure.                        */
                                              GA_Width,  PROPGADGETWIDTH,
                                              GA_Height, PROPGADGETHEIGHT,

                                              ICA_MAP,      prop2intmap, /* The prop gadget's attribute map */

                                              /* The rest of this gadget's attributes are defined by propgclass. */
                                              PGA_Total,     TOTAL,          /* This is the integer range of the prop gadget.  */
                                              PGA_Top,       INITIALVAL,     /* The initial integer value of the prop gadget.  */

                                              PGA_Visible,   VISIBLE, /* This determines how much of the prop gadget area is   */
                                                                      /* covered by the prop gadget's knob, or how much of     */
                                                                      /* the gadget's TOTAL range is taken up by the prop      */
                                                                      /* gadget's knob.                                        */

                                              PGA_NewLook,   TRUE,    /* Use new-look prop gadget imagery */
                                              TAG_END);
            if (!prop) goto fail;

            integer = (struct Gadget *)NewObject(NULL, (STRPTR) "strgclass",
                                                 GA_ID,      INTGADGET_ID,         /* Parameters for the Gadget structure     */
                                                 GA_Top,     (w->BorderTop) + 5L,
                                                 GA_Left,    (w->BorderLeft) + PROPGADGETWIDTH + 10L,
                                                 GA_Width,   MINWINDOWWIDTH -
                                                               (w->BorderLeft + w->BorderRight +
                                                               PROPGADGETWIDTH + 15L),
                                                 GA_Height,  INTGADGETHEIGHT,

                                                 ICA_MAP,    int2propmap,           /* The attribute map */
                                                 ICA_TARGET, prop,                  /* plus the target.  */

                                                         /* Th GA_Previous attribute is defined by gadgetclass and is used to */
                                                         /* wedge a new gadget into a list of gadget's linked by their        */
                                                         /* Gadget.NextGadget field.  When NewObject() creates this gadget,   */
                                                         /* it inserts the new gadget into this list behind the GA_Previous   */
                                                         /* gadget. This attribute is a pointer to the previous gadget        */
                                                         /* (struct Gadget *).  This attribute cannot be used to link new     */
                                                         /* gadgetsinto the gadget list of an open window or requester,       */
                                                 GA_Previous, prop,  /* use AddGList() instead.                               */

                                                 STRINGA_LongVal,  INITIALVAL, /* These attributes are defined by strgclass.  */
                                                 STRINGA_MaxChars, MAXCHARS,   /* The first contains the value of the         */
                                                 TAG_END);                     /* integer string gadget. The second is the    */
                                                                               /* maximum number of characters the user is    */
                                                                               /* allowed to type into the gadget.            */
            if (!integer) goto fail;

            SetGadgetAttrs(prop, w, NULL, /* Because the integer string gadget did not   */
                ICA_TARGET, integer,      /* exist when this example created the prop    */
                TAG_END);                 /* gadget, it had to wait to set the           */
                                          /* ICA_Target of the prop gadget.              */

            AddGList(w, prop, -1, -1, NULL);  /* Add the gadgets to the                  */
            RefreshGList(prop, w, NULL, -1);  /* window and display them.                */

            while (done == FALSE)     /* Wait for the user to click */
            {                         /* the window close gadget.   */
                WaitPort((struct MsgPort *)w->UserPort);
                while ( (msg = (struct IntuiMessage *) GetMsg((struct MsgPort *)w->UserPort)) )
                {
                    if (msg->Class == IDCMP_CLOSEWINDOW)
                        done = TRUE;
                    ReplyMsg((struct Message *)msg);
                }
            }
            RemoveGList(w, prop, -1);
fail:
            if (integer)
                DisposeObject(integer);
            if (prop)
                DisposeObject(prop);
            CloseWindow(w);
        }
        CloseLibrary(IntuitionBase);
    }
}
