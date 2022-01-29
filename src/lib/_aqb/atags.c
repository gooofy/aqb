#include "_aqb.h"

#include <stdarg.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

TAGITEM_t *TAGITEMS_ (ULONG ti_Tag, ...)
{
    DPRINTF ("TAGITEMS_: first ti_Tag=%ld\n", ti_Tag);

	int     num_tags = 1;
    va_list ap;

	if (ti_Tag)
	{
		va_start(ap, ti_Tag);
		va_arg (ap, ULONG);             // ignore first tag's data

        ULONG tag;
        do
        {
            num_tags++;
            tag  = va_arg (ap, ULONG);
            va_arg (ap, ULONG);         // ignore tag's data
            DPRINTF ("TAGITEMS_: tag=%ld\n", tag);
        } while (tag);

		va_end(ap);
	}

	DPRINTF ("TAGITEMS_: num_tags=%d\n", num_tags);

    ULONG tsize = 2*4*num_tags;
	TAGITEM_t *tags = (TAGITEM_t *) ALLOCATE_ (tsize, 0);
	if (!tags)
	{
		DPRINTF ("TAGITEMS_: out of memory!\n");
		ERROR (ERR_OUT_OF_MEMORY);
		return NULL;
	}

	DPRINTF ("TAGITEMS_: tags (tsize=%ld) allocated at 0x%08lx\n", tsize, tags);
    TAGITEM_t *ti = tags;
	ti->ti_Tag = ti_Tag;

	if (ti_Tag)
	{
		va_start(ap, ti_Tag);
		ti->ti_Data = va_arg (ap, ULONG);
        DPRINTF ("TAGITEMS_: set first TagItem: tag=%ld, data=%ld\n", ti->ti_Tag, ti->ti_Data);
		ti++;

        ULONG tag;
        do
        {
            tag  = va_arg (ap, ULONG);
            ti->ti_Tag = tag;
            if (tag)
                ti->ti_Data = va_arg (ap, ULONG);
            else
                ti->ti_Data = 0;
            DPRINTF ("TAGITEMS_: set next TagItem: tag=%ld, data=%ld\n", ti->ti_Tag, ti->ti_Data);
            ti++;
        } while (tag);

		va_end(ap);
	}

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

