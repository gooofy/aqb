#include "_aqb.h"

#include <clib/exec_protos.h>
#include <inline/exec.h>

#define MAX_TAGS    64

struct TagItem *_vatagitems (ULONG ti_Tag, va_list ap)
{
	int              num_tags = 1;
    static struct TagItem tagbuf[MAX_TAGS];

    DPRINTF ("_vatagitems: first ti_Tag=%ld\n", ti_Tag);
    tagbuf[0].ti_Tag = ti_Tag;

	if (ti_Tag)
	{
		tagbuf[0].ti_Data = va_arg (ap, ULONG);

        ULONG tag;
        do
        {
            tagbuf[num_tags].ti_Tag  = tag = va_arg (ap, ULONG);
            tagbuf[num_tags].ti_Data       = va_arg (ap, ULONG);
            num_tags++;
            DPRINTF ("_vatagitems: tag=%ld\n", tag);

            if (num_tags > MAX_TAGS)
            {
                DPRINTF ("TAGITEMS_: too many tags!\n");
                ERROR (ERR_OUT_OF_MEMORY);
                return NULL;
            }

        } while (tag);

		va_end(ap);
	}
    else
    {
		tagbuf[0].ti_Data = 0;
    }

	DPRINTF ("_vatagitems: num_tags=%d\n", num_tags);

    ULONG tsize = 2*4*num_tags;
	struct TagItem *tags = (struct TagItem *) ALLOCATE_ (tsize, 0);
	if (!tags)
	{
		DPRINTF ("TAGITEMS_: out of memory!\n");
		ERROR (ERR_OUT_OF_MEMORY);
		return NULL;
	}

    CopyMem (tagbuf, tags, tsize);

    return tags;
}

struct TagItem *TAGITEMS_ (ULONG ti_Tag, ...)
{
    DPRINTF ("TAGITEMS_: first ti_Tag=%ld\n", ti_Tag);

    va_list ap;

    va_start(ap, ti_Tag);
    struct TagItem *tags = _vatagitems (ti_Tag, ap);
    va_end(ap);

    return tags;
}

ULONG *TAGS_ (ULONG ti_Tag, ...)
{
    DPRINTF ("TAGS_: first ti_Tag=%ld\n", ti_Tag);

	int     num_tags = 1;
    va_list ap;

	if (ti_Tag)
	{
		va_start(ap, ti_Tag);

        ULONG tag;
        do
        {
            num_tags++;
            tag = va_arg (ap, ULONG);
            DPRINTF ("TAGS_: tag=%ld\n", tag);
        } while (tag);

		va_end(ap);
	}

	DPRINTF ("TAGS_: num_tags=%d\n", num_tags);

	ULONG *tags = (ULONG *) ALLOCATE_ (4 * num_tags, 0);
	if (!tags)
	{
		DPRINTF ("TAGS_: out of memory!\n");
		ERROR (ERR_OUT_OF_MEMORY);
		return NULL;
	}

    ULONG *t = tags;
    *t++ = ti_Tag;

	if (ti_Tag)
	{
		va_start(ap, ti_Tag);
        ULONG tag;
        do
        {
            tag = va_arg (ap, ULONG);
            *t++ = tag;
        } while (tag);

		va_end(ap);
	}

    return tags;
}

