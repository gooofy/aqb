OPTION EXPLICIT
OPTION PRIVATE

REM IMPORT OSExec

PUBLIC TYPE TagItem
    AS ULONG     ti_Tag
    AS ULONG     ti_Data
END TYPE

PUBLIC CONST AS ULONG TAG_DONE   = &H00000000
PUBLIC CONST AS ULONG TAG_END	 = &H00000000
PUBLIC CONST AS ULONG TAG_IGNORE = &H00000001
PUBLIC CONST AS ULONG TAG_MORE   = &H00000002
PUBLIC CONST AS ULONG TAG_SKIP   = &H00000003
PUBLIC CONST AS ULONG TAG_USER   = &H80000000

PUBLIC CONST AS LONG TAGFILTER_AND        = 0
PUBLIC CONST AS LONG TAGFILTER_NOT        = 1

PUBLIC CONST AS LONG MAP_REMOVE_NOT_FOUND = 0
PUBLIC CONST AS LONG MAP_KEEP_NOT_FOUND   = 1

PUBLIC TYPE ClockData
    AS UINTEGER     sec
    AS UINTEGER     min
    AS UINTEGER     hour
    AS UINTEGER     mday
    AS UINTEGER     month
    AS UINTEGER     year
    AS UINTEGER     wday
END TYPE

PUBLIC TYPE Hook
    REM FIXME? AS MinNode     h_MinNode
    AS ANY PTR            h_MinNode_mln_Succ
    AS ANY PTR            h_MinNode_mln_Pred

    AS FUNCTION AS ULONG  h_Entry
    AS FUNCTION AS ULONG  h_SubEntry

    AS ANY PTR            h_Data
END TYPE

PUBLIC TYPE NamedObject
    AS ANY PTR      no_Object
END TYPE

PUBLIC CONST AS UINTEGER ANO_NameSpace     = 4000
PUBLIC CONST AS UINTEGER ANO_UserSpace     = 4001
PUBLIC CONST AS UINTEGER ANO_Priority      = 4002
PUBLIC CONST AS UINTEGER ANO_Flags         = 4003

PUBLIC CONST AS UINTEGER NSB_NODUPS        = 0
PUBLIC CONST AS UINTEGER NSB_CASE          = 1

PUBLIC CONST AS UINTEGER NSF_NODUPS        =  1 SHL NSB_NODUPS
PUBLIC CONST AS UINTEGER NSF_CASE          =  1 SHL NSB_CASE

PUBLIC CONST AS UINTEGER PSTB_SIGNED       = 31
PUBLIC CONST AS UINTEGER PSTB_UNPACK       = 30
PUBLIC CONST AS UINTEGER PSTB_PACK         = 29
PUBLIC CONST AS UINTEGER PSTB_EXISTS       = 26

PUBLIC CONST AS UINTEGER PSTF_SIGNED       = 1 SHL PSTB_SIGNED
PUBLIC CONST AS UINTEGER PSTF_UNPACK       = 1 SHL PSTB_UNPACK
PUBLIC CONST AS UINTEGER PSTF_PACK         = 1 SHL PSTB_PACK
PUBLIC CONST AS UINTEGER PSTF_EXISTS       = 1 SHL PSTB_EXISTS

PUBLIC CONST AS ULONG    PKCTRL_PACKUNPACK = &H00000000
PUBLIC CONST AS ULONG    PKCTRL_PACKONLY   = &H40000000
PUBLIC CONST AS ULONG    PKCTRL_UNPACKONLY = &H20000000
PUBLIC CONST AS ULONG    PKCTRL_BYTE       = &H80000000
PUBLIC CONST AS ULONG    PKCTRL_WORD       = &H88000000
PUBLIC CONST AS ULONG    PKCTRL_LONG       = &H90000000
PUBLIC CONST AS ULONG    PKCTRL_UBYTE      = &H00000000
PUBLIC CONST AS ULONG    PKCTRL_UWORD      = &H08000000
PUBLIC CONST AS ULONG    PKCTRL_ULONG      = &H10000000
PUBLIC CONST AS ULONG    PKCTRL_BIT        = &H18000000
PUBLIC CONST AS ULONG    PKCTRL_FLIPBIT    = &H98000000

PUBLIC CONST AS UINTEGER PACK_ENDTABLE     = 0

PUBLIC EXTERN UtilityBase AS ANY PTR

PUBLIC DECLARE EXTERN FUNCTION AllocateTagItems     (numItems AS ULONG                                                    ) AS TagItem PTR LIB  -66 UtilityBase (d0)
PUBLIC DECLARE EXTERN SUB      Amiga2Date           (amigaTime AS ULONG, date AS ClockData PTR                            )                LIB -120 UtilityBase (d0, a0)
PUBLIC DECLARE EXTERN FUNCTION CallHookPkt          (hook AS Hook PTR, object AS ANY PTR , paramPacket AS ANY PTR         ) AS ULONG       LIB -102 UtilityBase (a0, a2, a1)
PUBLIC DECLARE EXTERN FUNCTION CheckDate            (date AS ClockData PTR                                                ) AS ULONG       LIB -132 UtilityBase (a0)
PUBLIC DECLARE EXTERN FUNCTION CloneTagItems        (tagList AS TagItem PTR                                               ) AS TagItem PTR LIB  -72 UtilityBase (a0)
PUBLIC DECLARE EXTERN FUNCTION Date2Amiga           (date AS ClockData PTR                                                ) AS ULONG       LIB -126 UtilityBase (a0)
PUBLIC DECLARE EXTERN SUB      FilterTagChanges     (newTagList AS TagItem PTR, oldTagList AS TagItem PTR, apply AS LONG  )                LIB  -54 UtilityBase (a0, a1, d0)
PUBLIC DECLARE EXTERN FUNCTION FilterTagItems       (tagList AS TagItem PTR, filterArray AS ULONG PTR, logic AS LONG      ) AS ULONG       LIB  -96 UtilityBase (a0, a1, d0)
PUBLIC DECLARE EXTERN FUNCTION FindTagItem          (tagVal AS ULONG, tagList AS TagItem PTR                              ) AS TagItem PTR LIB  -30 UtilityBase (d0, a0)
PUBLIC DECLARE EXTERN SUB      FreeTagItems         (tagList AS TagItem PTR                                               )                LIB  -78 UtilityBase (a0)
PUBLIC DECLARE EXTERN FUNCTION GetTagData           (tagVal AS ULONG, defaultVal AS ULONG, tagList AS TagItem PTR         ) AS ULONG       LIB  -36 UtilityBase (d0, d1, a0)
PUBLIC DECLARE EXTERN SUB      MapTags              (tagList AS TagItem PTR, mapList AS TagItem PTR, includeMiss AS LONG  )                LIB  -60 UtilityBase (a0, a1, d0)
PUBLIC DECLARE EXTERN FUNCTION NextTagItem          (tagListPtr AS ANY PTR                                                ) AS TagItem PTR LIB  -48 UtilityBase (a0)
PUBLIC DECLARE EXTERN FUNCTION PackBoolTags         (initialFlags AS ULONG, tagList AS TagItem PTR, boolMap AS TagItem PTR) AS ULONG       LIB  -42 UtilityBase (d0, a0, a1)
PUBLIC DECLARE EXTERN SUB      RefreshTagItemClones (cloneList AS TagItem PTR, origList AS TagItem PTR                    )                LIB  -84 UtilityBase (a0, a1)
PUBLIC DECLARE EXTERN FUNCTION SDivMod32            (dividend AS LONG, divisor AS LONG                                    ) AS LONG        LIB -150 UtilityBase (d0, d1)
PUBLIC DECLARE EXTERN FUNCTION SMult32              (factor1 AS LONG, factor2 AS LONG                                     ) AS LONG        LIB -138 UtilityBase (d0, d1)
PUBLIC DECLARE EXTERN FUNCTION Stricmp              (string1 AS string, string2 AS string                                 ) AS LONG        LIB -162 UtilityBase (a0, a1)
PUBLIC DECLARE EXTERN FUNCTION Strnicmp             (string1 AS string, string2 AS string, length AS LONG                 ) AS LONG        LIB -168 UtilityBase (a0, a1, d0)
PUBLIC DECLARE EXTERN FUNCTION TagInArray           (tagVal AS ULONG, tagArray AS ULONG PTR                               ) AS BOOLEAN     LIB  -90 UtilityBase (d0, a0)
PUBLIC DECLARE EXTERN FUNCTION ToLower              (character AS UBYTE                                                   ) AS UBYTE       LIB -180 UtilityBase (d0)
PUBLIC DECLARE EXTERN FUNCTION ToUpper              (character AS UBYTE                                                   ) AS UBYTE       LIB -174 UtilityBase (d0)
PUBLIC DECLARE EXTERN FUNCTION UDivMod32            (dividend AS ULONG, divisor AS ULONG                                  ) AS ULONG       LIB -156 UtilityBase (d0, d1)
PUBLIC DECLARE EXTERN FUNCTION UMult32              (factor1 AS ULONG, factor2 AS ULONG                                   ) AS ULONG       LIB -144 UtilityBase (d0, d1)
