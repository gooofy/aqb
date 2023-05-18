
# OSUtility

OSUtility is a low level utility.library AQB interface.

```
CONST AS LONG TAGFILTER_AND        = 0
CONST AS LONG TAGFILTER_NOT        = 1

CONST AS LONG MAP_REMOVE_NOT_FOUND = 0
CONST AS LONG MAP_KEEP_NOT_FOUND   = 1

TYPE ClockData
    AS UINTEGER     sec
    AS UINTEGER     min
    AS UINTEGER     hour
    AS UINTEGER     mday
    AS UINTEGER     month
    AS UINTEGER     year
    AS UINTEGER     wday
END TYPE

TYPE Hook
    REM FIXME? AS MinNode     h_MinNode
    AS VOID PTR           h_MinNode_mln_Succ
    AS VOID PTR           h_MinNode_mln_Pred

    AS FUNCTION AS ULONG  h_Entry
    AS FUNCTION AS ULONG  h_SubEntry

    AS VOID PTR           h_Data
END TYPE

PUBLIC EXTERN UtilityBase AS VOID PTR

DECLARE FUNCTION AllocateTagItems     (numItems AS ULONG                                                    ) AS TagItem PTR LIB  -66 UtilityBase (d0)
DECLARE SUB      Amiga2Date           (amigaTime AS ULONG, date AS ClockData PTR                            )                LIB -120 UtilityBase (d0, a0)
DECLARE FUNCTION CallHookPkt          (hook AS Hook PTR, object AS VOID PTR, paramPacket AS VOID PTR        ) AS ULONG       LIB -102 UtilityBase (a0, a2, a1)
DECLARE FUNCTION CheckDate            (date AS ClockData PTR                                                ) AS ULONG       LIB -132 UtilityBase (a0)
DECLARE FUNCTION CloneTagItems        (tagList AS TagItem PTR                                               ) AS TagItem PTR LIB  -72 UtilityBase (a0)
DECLARE FUNCTION Date2Amiga           (date AS ClockData PTR                                                ) AS ULONG       LIB -126 UtilityBase (a0)
DECLARE SUB      FilterTagChanges     (newTagList AS TagItem PTR, oldTagList AS TagItem PTR, apply AS LONG  )                LIB  -54 UtilityBase (a0, a1, d0)
DECLARE FUNCTION FilterTagItems       (tagList AS TagItem PTR, filterArray AS ULONG PTR, logic AS LONG      ) AS ULONG       LIB  -96 UtilityBase (a0, a1, d0)
DECLARE FUNCTION FindTagItem          (tagVal AS ULONG, tagList AS TagItem PTR                              ) AS TagItem PTR LIB  -30 UtilityBase (d0, a0)
DECLARE SUB      FreeTagItems         (tagList AS TagItem PTR                                               )                LIB  -78 UtilityBase (a0)
DECLARE FUNCTION GetTagData           (tagVal AS ULONG, defaultVal AS ULONG, tagList AS TagItem PTR         ) AS ULONG       LIB  -36 UtilityBase (d0, d1, a0)
DECLARE SUB      MapTags              (tagList AS TagItem PTR, mapList AS TagItem PTR, includeMiss AS LONG  )                LIB  -60 UtilityBase (a0, a1, d0)
DECLARE FUNCTION NextTagItem          (tagListPtr AS VOID PTR                                               ) AS TagItem PTR LIB  -48 UtilityBase (a0)
DECLARE FUNCTION PackBoolTags         (initialFlags AS ULONG, tagList AS TagItem PTR, boolMap AS TagItem PTR) AS ULONG       LIB  -42 UtilityBase (d0, a0, a1)
DECLARE SUB      RefreshTagItemClones (cloneList AS TagItem PTR, origList AS TagItem PTR                    )                LIB  -84 UtilityBase (a0, a1)
DECLARE FUNCTION SDivMod32            (dividend AS LONG, divisor AS LONG                                    ) AS LONG        LIB -150 UtilityBase (d0, d1)
DECLARE FUNCTION SMult32              (factor1 AS LONG, factor2 AS LONG                                     ) AS LONG        LIB -138 UtilityBase (d0, d1)
DECLARE FUNCTION Stricmp              (string1 AS string, string2 AS string                                 ) AS LONG        LIB -162 UtilityBase (a0, a1)
DECLARE FUNCTION Strnicmp             (string1 AS string, string2 AS string, length AS LONG                 ) AS LONG        LIB -168 UtilityBase (a0, a1, d0)
DECLARE FUNCTION TagInArray           (tagVal AS ULONG, tagArray AS ULONG PTR                               ) AS BOOLEAN     LIB  -90 UtilityBase (d0, a0)
DECLARE FUNCTION ToLower              (character AS UBYTE                                                   ) AS UBYTE       LIB -180 UtilityBase (d0)
DECLARE FUNCTION ToUpper              (character AS UBYTE                                                   ) AS UBYTE       LIB -174 UtilityBase (d0)
DECLARE FUNCTION UDivMod32            (dividend AS ULONG, divisor AS ULONG                                  ) AS ULONG       LIB -156 UtilityBase (d0, d1)
DECLARE FUNCTION UMult32              (factor1 AS ULONG, factor2 AS ULONG                                   ) AS ULONG       LIB -144 UtilityBase (d0, d1)
```

