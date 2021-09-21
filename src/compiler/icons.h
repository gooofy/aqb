#include <workbench/workbench.h>

USHORT aqbsrc1Data[132] =
{
	0x0000,0x0000,0x0000,0x7FFF,0xFFC0,0x0000,0x58CC,0x78D0,
	0x0000,0x7BB5,0xBDDC,0x0000,0x59B4,0x7DDF,0x0000,0x7BCD,
	0x78DF,0xC000,0x5FFF,0xFFC0,0x0000,0x7FE3,0x18B4,0x7800,
	0x5FED,0x6D96,0xE800,0x7FE3,0x1DA6,0xF800,0x5FEF,0x58B6,
	0xE800,0x7FFF,0xFFFF,0xF800,0x5B45,0x471F,0xE800,0x794E,
	0xEFBF,0xF800,0x5A5E,0xEFBF,0xE800,0x7B45,0x6F1F,0xF800,
	0x5FFF,0xFFFF,0xE800,0x7FFF,0xFFFF,0xF800,0x5FFF,0xFFFF,
	0xE800,0x7FFF,0xFFFF,0xF800,0x5FFF,0xFFFF,0xE800,0x0000,
	0x0000,0x0000,

	0xFFFF,0xFFC0,0x0000,0x8000,0x0030,0x0000,0xA000,0x002C,
	0x0000,0x8000,0x0023,0x0000,0xA000,0x0020,0xC000,0x8000,
	0x0020,0x3000,0xA000,0x003F,0xFC00,0x8000,0x0000,0x0400,
	0xA000,0x0000,0x1400,0x8000,0x0000,0x0400,0xA000,0x0000,
	0x1400,0x8000,0x0000,0x0400,0xA000,0x0000,0x1400,0x8000,
	0x0000,0x0400,0xA000,0x0000,0x1400,0x8000,0x0000,0x0400,
	0xA000,0x0000,0x1400,0x8000,0x0000,0x0400,0xA000,0x0000,
	0x1400,0x8000,0x0000,0x0400,0xA000,0x0000,0x1400,0xFFFF,
	0xFFFF,0xFC00
};

struct Image aqbsrc1Image =
{
	0,0,
	38,22,2,
	(USHORT *)&aqbsrc1Data[0],
	0x03,0x00,
	(struct Image *)NULL
};

    UWORD		do_Magic;
    UWORD		do_Version;
    struct Gadget	do_Gadget;
    UBYTE		do_Type;
    char *		do_DefaultTool;
    char **		do_ToolTypes;

struct DiskObject aqbsrcIcon =
{
	/* UWORD    do_Magic         = */ WB_DISKMAGIC,
	/* UWORD    do_Version       = */ WB_DISKVERSION,
	/* struct Gadget do_Gadget   = */
	{
		NULL,
		24,15,
		38,23,
		0x0005,
		0x0003,
		0x0001,
		(APTR)&aqbsrc1Image,
		(APTR)NULL,
		/* struct IntuiText *GadgetText =*/NULL,
		0l,
		NULL,
		0l,
		NULL
	},
	/* UBYTE    do_Type          = */ WBPROJECT,
	/* char *	do_DefaultTool   = */ (STRPTR) "AQB:aqb",
	/* char **	do_ToolTypesNULL = */ NULL,
	/* LONG		do_CurrentX      = */ NO_ICON_POSITION,
	/* LONG		do_CurrentY      = */ NO_ICON_POSITION,
	/* struct DrawerData *       = */ NULL,
	/* char *	do_ToolWindow    = */ NULL,
	/* LONG		do_StackSize     = */ 100000	/* FIXME */
};

#include <workbench/workbench.h>

USHORT aqbbin1Data[192] =
{
	0x0000,0x0000,0x0000,0x0000,0x0007,0xFFFF,0xFFFF,0xFC00,
	0x001F,0xFFFF,0xFFFF,0xF300,0x007F,0xFFFF,0xFFFF,0xCF00,
	0x01FF,0xFFFF,0xFFFF,0x3F00,0x07FF,0xFFFF,0xFFFC,0xFF00,
	0x0000,0x0000,0x0001,0xFF00,0x1FFF,0xFFFF,0xFFF9,0xFF00,
	0x18EE,0x38ED,0xC719,0xFF00,0x176D,0xD76D,0xBAE9,0xFF00,
	0x176D,0xD76D,0xBAE9,0xFF00,0x176D,0xD76D,0xBAE9,0xFF00,
	0x18EE,0x38ED,0xC719,0xFF00,0x1FFF,0xFFFF,0xFFF9,0xFF00,
	0x1FFF,0xFFFF,0xFFF9,0xFF00,0x1FFF,0xFFFF,0xFFF9,0xFF00,
	0x1FFF,0xFFFF,0xFFF9,0xFF00,0x1FFF,0xFFFF,0xFFF9,0xFF00,
	0x1FFF,0xFFFF,0xFFF9,0xFE00,0x1FFF,0xFFFF,0xFFF9,0xF800,
	0x1FFF,0xFFFF,0xFFF9,0xE000,0x1FFF,0xFFFF,0xFFF9,0x8000,
	0x1FFF,0xFFFF,0xFFF8,0x0000,0x0000,0x0000,0x0000,0x0000,

	0x0007,0xFFFF,0xFFFF,0xFFC0,0x0018,0x0000,0x0000,0x03C0,
	0x0060,0x0000,0x0000,0x0CC0,0x0180,0x0000,0x0000,0x30C0,
	0x0600,0x0000,0x0000,0xC0C0,0x1800,0x0000,0x0003,0x00C0,
	0x7FFF,0xFFFF,0xFFFE,0x00C0,0x6000,0x0000,0x0006,0x00C0,
	0x6000,0x0000,0x0006,0x00C0,0x6000,0x0000,0x0006,0x00C0,
	0x6000,0x0000,0x0006,0x00C0,0x6000,0x0000,0x0006,0x00C0,
	0x6000,0x0000,0x0006,0x00C0,0x6000,0x0000,0x0006,0x00C0,
	0x6000,0x0000,0x0006,0x00C0,0x6000,0x0000,0x0006,0x00C0,
	0x6000,0x0000,0x0006,0x00C0,0x6000,0x0000,0x0006,0x00C0,
	0x6000,0x0000,0x0006,0x0180,0x6000,0x0000,0x0006,0x0600,
	0x6000,0x0000,0x0006,0x1800,0x6000,0x0000,0x0006,0x6000,
	0x6000,0x0000,0x0007,0x8000,0x7FFF,0xFFFF,0xFFFE,0x0000
};

struct Image aqbbin1Image =
{
	0,0,
	58,24,2,
	(USHORT *)&aqbbin1Data[0],
	0x03,0x00,
	(struct Image *)NULL
};

struct DiskObject aqbbinIcon =
{
	WB_DISKMAGIC,
	WB_DISKVERSION,
    {
        NULL,
        11,15,
        58,25,
        0x0005,
        0x0003,
        0x0001,
        (APTR)&aqbbin1Image,
        NULL,
        NULL,
        0l,
        NULL,
        0l,
        NULL
    },
	WBTOOL,
	NULL,
	NULL,
	NO_ICON_POSITION,
	NO_ICON_POSITION,
	NULL,
	NULL,
	65536 /* FIXME */
};
