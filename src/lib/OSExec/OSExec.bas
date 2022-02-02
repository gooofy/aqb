OPTION EXPLICIT

CONST AS ULONG ACPU_BusErr           = &H80000002
CONST AS ULONG ACPU_AddressErr       = &H80000003
CONST AS ULONG ACPU_InstErr          = &H80000004
CONST AS ULONG ACPU_DivZero          = &H80000005
CONST AS ULONG ACPU_CHK              = &H80000006
CONST AS ULONG ACPU_TRAPV            = &H80000007
CONST AS ULONG ACPU_PrivErr          = &H80000008
CONST AS ULONG ACPU_Trace            = &H80000009
CONST AS ULONG ACPU_LineA            = &H8000000A
CONST AS ULONG ACPU_LineF            = &H8000000B
CONST AS ULONG ACPU_Format           = &H8000000E
CONST AS ULONG ACPU_Spurious         = &H80000018

CONST AS ULONG ACPU_AutoVec1         = &H80000019
CONST AS ULONG ACPU_AutoVec2         = &H8000001A
CONST AS ULONG ACPU_AutoVec3         = &H8000001B
CONST AS ULONG ACPU_AutoVec4         = &H8000001C
CONST AS ULONG ACPU_AutoVec5         = &H8000001D
CONST AS ULONG ACPU_AutoVec6         = &H8000001E
CONST AS ULONG ACPU_AutoVec7         = &H8000001F

CONST AS ULONG AT_DeadEnd            = &H80000000
CONST AS ULONG AT_Recovery           = &H00000000
CONST AS ULONG AG_NoMemory           = &H00010000
CONST AS ULONG AG_MakeLib            = &H00020000
CONST AS ULONG AG_OpenLib            = &H00030000
CONST AS ULONG AG_OpenDev            = &H00040000
CONST AS ULONG AG_OpenRes            = &H00050000
CONST AS ULONG AG_IOError            = &H00060000
CONST AS ULONG AG_NoSignal           = &H00070000
CONST AS ULONG AG_BadParm            = &H00080000
CONST AS ULONG AG_CloseLib           = &H00090000
CONST AS ULONG AG_CloseDev           = &H000A0000
CONST AS ULONG AG_ProcCreate         = &H000B0000
CONST AS ULONG AO_ExecLib            = &H00008001
CONST AS ULONG AO_GraphicsLib        = &H00008002
CONST AS ULONG AO_LayersLib          = &H00008003
CONST AS ULONG AO_Intuition          = &H00008004
CONST AS ULONG AO_MathLib            = &H00008005
CONST AS ULONG AO_DOSLib             = &H00008007
CONST AS ULONG AO_RAMLib             = &H00008008
CONST AS ULONG AO_IconLib            = &H00008009
CONST AS ULONG AO_ExpansionLib       = &H0000800A
CONST AS ULONG AO_DiskfontLib        = &H0000800B
CONST AS ULONG AO_UtilityLib         = &H0000800C
CONST AS ULONG AO_KeyMapLib          = &H0000800D
CONST AS ULONG AO_AudioDev           = &H00008010
CONST AS ULONG AO_ConsoleDev         = &H00008011
CONST AS ULONG AO_GamePortDev        = &H00008012
CONST AS ULONG AO_KeyboardDev        = &H00008013
CONST AS ULONG AO_TrackDiskDev       = &H00008014
CONST AS ULONG AO_TimerDev           = &H00008015
CONST AS ULONG AO_CIARsrc            = &H00008020
CONST AS ULONG AO_DiskRsrc           = &H00008021
CONST AS ULONG AO_MiscRsrc           = &H00008022
CONST AS ULONG AO_BootStrap          = &H00008030
CONST AS ULONG AO_Workbench          = &H00008031
CONST AS ULONG AO_DiskCopy           = &H00008032
CONST AS ULONG AO_GadTools           = &H00008033
CONST AS ULONG AO_Unknown            = &H00008035
CONST AS ULONG AN_ExecLib            = &H01000000
CONST AS ULONG AN_ExcptVect          = &H01000001
CONST AS ULONG AN_BaseChkSum         = &H01000002
CONST AS ULONG AN_LibChkSum          = &H01000003
CONST AS ULONG AN_MemCorrupt         = &H81000005
CONST AS ULONG AN_IntrMem            = &H81000006
CONST AS ULONG AN_InitAPtr           = &H01000007
CONST AS ULONG AN_SemCorrupt         = &H01000008
CONST AS ULONG AN_FreeTwice          = &H01000009
CONST AS ULONG AN_BogusExcpt         = &H8100000A
CONST AS ULONG AN_IOUsedTwice        = &H0100000B
CONST AS ULONG AN_MemoryInsane       = &H0100000C
CONST AS ULONG AN_IOAfterClose       = &H0100000D
CONST AS ULONG AN_StackProbe         = &H0100000E
CONST AS ULONG AN_BadFreeAddr        = &H0100000F
CONST AS ULONG AN_BadSemaphore       = &H01000010
CONST AS ULONG AN_GraphicsLib        = &H02000000
CONST AS ULONG AN_GfxNoMem           = &H82010000
CONST AS ULONG AN_GfxNoMemMspc       = &H82010001
CONST AS ULONG AN_LongFrame          = &H82010006
CONST AS ULONG AN_ShortFrame         = &H82010007
CONST AS ULONG AN_TextTmpRas         = &H02010009
CONST AS ULONG AN_BltBitMap          = &H8201000A
CONST AS ULONG AN_RegionMemory       = &H8201000B
CONST AS ULONG AN_MakeVPort          = &H82010030
CONST AS ULONG AN_GfxNewError        = &H0200000C
CONST AS ULONG AN_GfxFreeError       = &H0200000D
CONST AS ULONG AN_GfxNoLCM           = &H82011234
CONST AS ULONG AN_ObsoleteFont       = &H02000401
CONST AS ULONG AN_LayersLib          = &H03000000
CONST AS ULONG AN_LayersNoMem        = &H83010000
CONST AS ULONG AN_Intuition          = &H04000000
CONST AS ULONG AN_GadgetType         = &H84000001
CONST AS ULONG AN_BadGadget          = &H04000001
CONST AS ULONG AN_CreatePort         = &H84010002
CONST AS ULONG AN_ItemAlloc          = &H04010003
CONST AS ULONG AN_SubAlloc           = &H04010004
CONST AS ULONG AN_PlaneAlloc         = &H84010005
CONST AS ULONG AN_ItemBoxTop         = &H84000006
CONST AS ULONG AN_OpenScreen         = &H84010007
CONST AS ULONG AN_OpenScrnRast       = &H84010008
CONST AS ULONG AN_SysScrnType        = &H84000009
CONST AS ULONG AN_AddSWGadget        = &H8401000A
CONST AS ULONG AN_OpenWindow         = &H8401000B
CONST AS ULONG AN_BadState           = &H8400000C
CONST AS ULONG AN_BadMessage         = &H8400000D
CONST AS ULONG AN_WeirdEchoa         = &H8400000E
CONST AS ULONG AN_NoConsole          = &H8400000F
CONST AS ULONG AN_NoISem             = &H04000010
CONST AS ULONG AN_ISemOrder          = &H04000011
CONST AS ULONG AN_MathLib            = &H05000000
CONST AS ULONG AN_DOSLib             = &H07000000
CONST AS ULONG AN_StartMem           = &H07010001
CONST AS ULONG AN_EndTask            = &H07000002
CONST AS ULONG AN_QPktFail           = &H07000003
CONST AS ULONG AN_AsyncPkt           = &H07000004
CONST AS ULONG AN_FreeVec            = &H07000005
CONST AS ULONG AN_DiskBlkSeq         = &H07000006
CONST AS ULONG AN_BitMap             = &H07000007
CONST AS ULONG AN_KeyFree            = &H07000008
CONST AS ULONG AN_BadChkSum          = &H07000009
CONST AS ULONG AN_DiskError          = &H0700000A
CONST AS ULONG AN_KeyRange           = &H0700000B
CONST AS ULONG AN_BadOverlay         = &H0700000C
CONST AS ULONG AN_BadInitFunc        = &H0700000D
CONST AS ULONG AN_FileReclosed       = &H0700000E
CONST AS ULONG AN_RAMLib             = &H08000000
CONST AS ULONG AN_BadSegList         = &H08000001
CONST AS ULONG AN_IconLib            = &H09000000
CONST AS ULONG AN_ExpansionLib       = &H0A000000
CONST AS ULONG AN_BadExpansionFree   = &H0A000001
CONST AS ULONG AN_DiskfontLib        = &H0B000000
CONST AS ULONG AN_AudioDev           = &H10000000
CONST AS ULONG AN_ConsoleDev         = &H11000000
CONST AS ULONG AN_NoWindow           = &H11000001
CONST AS ULONG AN_GamePortDev        = &H12000000
CONST AS ULONG AN_KeyboardDev        = &H13000000
CONST AS ULONG AN_TrackDiskDev       = &H14000000
CONST AS ULONG AN_TDCalibSeek        = &H14000001
CONST AS ULONG AN_TDDelay            = &H14000002
CONST AS ULONG AN_TimerDev           = &H15000000
CONST AS ULONG AN_TMBadReq           = &H15000001
CONST AS ULONG AN_TMBadSupply        = &H15000002
CONST AS ULONG AN_CIARsrc            = &H20000000
CONST AS ULONG AN_DiskRsrc           = &H21000000
CONST AS ULONG AN_DRHasDisk          = &H21000001
CONST AS ULONG AN_DRIntNoAct         = &H21000002
CONST AS ULONG AN_MiscRsrc           = &H22000000
CONST AS ULONG AN_BootStrap          = &H30000000
CONST AS ULONG AN_BootError          = &H30000001
CONST AS ULONG AN_Workbench          = &H31000000
CONST AS ULONG AN_NoFonts            = &HB1000001
CONST AS ULONG AN_WBBadStartupMsg1   = &H31000001
CONST AS ULONG AN_WBBadStartupMsg2   = &H31000002
CONST AS ULONG AN_WBBadIOMsg         = &H31000003
CONST AS ULONG AN_WBReLayoutToolMenu = &HB1010009
CONST AS ULONG AN_DiskCopy           = &H32000000
CONST AS ULONG AN_GadTools           = &H33000000
CONST AS ULONG AN_UtilityLib         = &H34000000
CONST AS ULONG AN_Unknown            = &H35000000

TYPE AVLNode
    AS ULONG     reserved(4)
END TYPE

TYPE Node
    AS Node PTR    ln_Succ
    AS Node PTR    ln_Pred
    AS UBYTE       ln_Type
    AS BYTE        ln_Pri
    AS STRING      ln_Name
END TYPE

TYPE MinNode
    AS MinNode PTR    mln_Succ
    AS MinNode PTR    mln_Pred
END TYPE

CONST AS UBYTE NT_UNKNOWN       = 0
CONST AS UBYTE NT_TASK          = 1
CONST AS UBYTE NT_INTERRUPT     = 2
CONST AS UBYTE NT_DEVICE        = 3
CONST AS UBYTE NT_MSGPORT       = 4
CONST AS UBYTE NT_MESSAGE       = 5
CONST AS UBYTE NT_FREEMSG       = 6
CONST AS UBYTE NT_REPLYMSG      = 7
CONST AS UBYTE NT_RESOURCE      = 8
CONST AS UBYTE NT_LIBRARY       = 9
CONST AS UBYTE NT_MEMORY        = 10
CONST AS UBYTE NT_SOFTINT       = 11
CONST AS UBYTE NT_FONT          = 12
CONST AS UBYTE NT_PROCESS       = 13
CONST AS UBYTE NT_SEMAPHORE     = 14
CONST AS UBYTE NT_SIGNALSEM     = 15
CONST AS UBYTE NT_BOOTNODE      = 16
CONST AS UBYTE NT_KICKMEM       = 17
CONST AS UBYTE NT_GRAPHICS      = 18
CONST AS UBYTE NT_DEATHMESSAGE  = 19
CONST AS UBYTE NT_USER          = 254
CONST AS UBYTE NT_EXTENDED      = 255

CONST AS INTEGER LIB_VECTSIZE   = 6
CONST AS INTEGER LIB_RESERVED   = 4
CONST AS INTEGER LIB_BASE       = -LIB_VECTSIZE
CONST AS INTEGER LIB_USERDEF    = LIB_BASE-(LIB_RESERVED*LIB_VECTSIZE)
CONST AS INTEGER LIB_NONSTD     = LIB_USERDEF

CONST AS INTEGER LIB_OPEN       = -6
CONST AS INTEGER LIB_CLOSE      = -12
CONST AS INTEGER LIB_EXPUNGE    = -18
CONST AS INTEGER LIB_EXTFUNC    = -24

TYPE Library
    AS Node         lib_Node
    AS UBYTE        lib_Flags
    AS UBYTE        lib_pad
    AS UINTEGER     lib_NegSize
    AS UINTEGER     lib_PosSize
    AS UINTEGER     lib_Version
    AS UINTEGER     lib_Revision
    AS STRING       lib_IdString
    AS ULONG        lib_Sum
    AS UINTEGER     lib_OpenCnt
END TYPE

CONST AS UBYTE LIBF_SUMMING = 1 SHL 0
CONST AS UBYTE LIBF_CHANGED = 1 SHL 1
CONST AS UBYTE LIBF_SUMUSED = 1 SHL 2
CONST AS UBYTE LIBF_DELEXP  = 1 SHL 3

TYPE Device
    AS Library     dd_Library
END TYPE

TYPE List
    AS Node PTR  lh_Head
    AS Node PTR  lh_Tail
    AS Node PTR  lh_TailPred
    AS UBYTE     lh_Type
    AS UBYTE     l_pad
END TYPE

TYPE MinList
    AS MinNode PTR    mlh_Head
    AS MinNode PTR    mlh_Tail
    AS MinNode PTR    mlh_TailPred
END TYPE

TYPE MsgPort
    AS Node        mp_Node
    AS UBYTE       mp_Flags
    AS UBYTE       mp_SigBit
    AS void PTR    mp_SigTask
    AS List        mp_MsgList
END TYPE

CONST AS UBYTE PF_ACTION         = 3
CONST AS UBYTE PA_SIGNAL         = 0
CONST AS UBYTE PA_SOFTINT        = 1
CONST AS UBYTE PA_IGNORE         = 2

TYPE Message
    AS Node         mn_Node
    AS MsgPort PTR  mn_ReplyPort
    AS UINTEGER     mn_Length
END TYPE

TYPE Unit
    AS MsgPort      unit_MsgPort
    AS UBYTE        unit_flags
    AS UBYTE        unit_pad
    AS UINTEGER     unit_OpenCnt
END TYPE

CONST AS UBYTE UNITF_ACTIVE      = 1
CONST AS UBYTE UNITF_INTASK      = 2

CONST AS BYTE IOERR_OPENFAIL     = -1
CONST AS BYTE IOERR_ABORTED      = -2
CONST AS BYTE IOERR_NOCMD        = -3
CONST AS BYTE IOERR_BADLENGTH    = -4
CONST AS BYTE IOERR_BADADDRESS   = -5
CONST AS BYTE IOERR_UNITBUSY     = -6
CONST AS BYTE IOERR_SELFTEST     = -7

TYPE Interrupt
    AS Node     is_Node
    AS VOID PTR is_Data
    AS SUB      is_Code
END TYPE

TYPE IntVector
    AS VOID PTR    iv_Data
    AS SUB         iv_Code
    AS Node PTR    iv_Node
END TYPE

TYPE SoftIntList
    AS List         sh_List
    AS UINTEGER     sh_Pad
END TYPE

CONST AS UBYTE SIH_PRIMASK = &Hf0

CONST AS UINTEGER INTB_NMI = 15
CONST AS UINTEGER INTF_NMI = &H8000

TYPE Task
    AS Node         tc_Node
    AS UBYTE        tc_Flags
    AS UBYTE        tc_State
    AS BYTE         tc_IDNestCnt
    AS BYTE         tc_TDNestCnt
    AS ULONG        tc_SigAlloc
    AS ULONG        tc_SigWait
    AS ULONG        tc_SigRecvd
    AS ULONG        tc_SigExcept
    AS UINTEGER     tc_TrapAlloc
    AS UINTEGER     tc_TrapAble
    AS VOID PTR     tc_ExceptData
    AS VOID PTR     tc_ExceptCode
    AS VOID PTR     tc_TrapData
    AS VOID PTR     tc_TrapCode
    AS VOID PTR     tc_SPReg
    AS VOID PTR     tc_SPLower
    AS VOID PTR     tc_SPUpper
    AS SUB          tc_Switch
    AS SUB          tc_Launch
    AS List         tc_MemEntry
    AS VOID PTR     tc_UserData
END TYPE

TYPE StackSwapStruct
    AS VOID PTR     stk_Lower
    AS ULONG        stk_Upper
    AS VOID PTR     stk_Pointer
END TYPE

CONST AS UBYTE TB_PROCTIME    = 0
CONST AS UBYTE TB_ETASK       = 3
CONST AS UBYTE TB_STACKCHK    = 4
CONST AS UBYTE TB_EXCEPT      = 5
CONST AS UBYTE TB_SWITCH      = 6
CONST AS UBYTE TB_LAUNCH      = 7

CONST AS UBYTE TF_PROCTIME    = 1 SHL 0
CONST AS UBYTE TF_ETASK       = 1 SHL 3
CONST AS UBYTE TF_STACKCHK    = 1 SHL 4
CONST AS UBYTE TF_EXCEPT      = 1 SHL 5
CONST AS UBYTE TF_SWITCH      = 1 SHL 6
CONST AS UBYTE TF_LAUNCH      = 1 SHL 7

CONST AS UBYTE TS_INVALID     = 0
CONST AS UBYTE TS_ADDED       = 1
CONST AS UBYTE TS_RUN         = 2
CONST AS UBYTE TS_READY       = 3
CONST AS UBYTE TS_WAIT        = 4
CONST AS UBYTE TS_EXCEPT      = 5
CONST AS UBYTE TS_REMOVED     = 6

CONST AS UBYTE SIGB_ABORT     = 0
CONST AS UBYTE SIGB_CHILD     = 1
CONST AS UBYTE SIGB_BLIT      = 4
CONST AS UBYTE SIGB_SINGLE    = 4
CONST AS UBYTE SIGB_INTUITION = 5
CONST AS UBYTE SIGB_NET       = 7
CONST AS UBYTE SIGB_DOS       = 8

CONST AS UBYTE SIGF_ABORT     = 1 SHL 0
CONST AS UBYTE SIGF_CHILD     = 1 SHL 1
CONST AS UBYTE SIGF_BLIT      = 1 SHL 4
CONST AS UBYTE SIGF_SINGLE    = 1 SHL 4
CONST AS UBYTE SIGF_INTUITION = 1 SHL 5
CONST AS UBYTE SIGF_NET       = 1 SHL 7
CONST AS UBYTE SIGF_DOS       = 1 SHL 8

TYPE ExecBase
    AS Library     LibNode
    AS UINTEGER    SoftVer
    AS INTEGER     LowMemChkSum
    AS ULONG       ChkBase
    AS VOID PTR    ColdCapture
    AS VOID PTR    CoolCapture
    AS VOID PTR    WarmCapture
    AS VOID PTR    SysStkUpper
    AS VOID PTR    SysStkLower
    AS ULONG       MaxLocMem
    AS VOID PTR    DebugEntry
    AS VOID PTR    DebugData
    AS VOID PTR    AlertData
    AS VOID PTR    MaxExtMem
    AS UINTEGER    ChkSum
    AS IntVector   IntVects(15)
    AS Task PTR    ThisTask
    AS ULONG       IdleCount
    AS ULONG       DispCount
    AS UINTEGER    Quantum
    AS UINTEGER    Elapsed
    AS UINTEGER    SysFlags
    AS BYTE        IDNestCnt
    AS BYTE        TDNestCnt
    AS UINTEGER    AttnFlags
    AS UINTEGER    AttnResched
    AS VOID PTR    ResModules
    AS VOID PTR    TaskTrapCode
    AS VOID PTR    TaskExceptCode
    AS VOID PTR    TaskExitCode
    AS ULONG       TaskSigAlloc
    AS UINTEGER    TaskTrapAlloc
    AS List        MemList
    AS List        ResourceList
    AS List        DeviceList
    AS List        IntrList
    AS List        LibList
    AS List        PortList
    AS List        TaskReady
    AS List        TaskWait
    AS SoftIntList SoftInts(4)
    AS LONG        LastAlert(3)
    AS UBYTE       VBlankFrequency
    AS UBYTE       PowerSupplyFrequency
    AS List        SemaphoreList
    AS VOID PTR    KickMemPtr
    AS VOID PTR    KickTagPtr
    AS VOID PTR    KickCheckSum
    AS UINTEGER    ex_Pad0
    AS ULONG       ex_LaunchPoint
    AS VOID PTR    ex_RamLibPrivate
    AS ULONG       ex_EClockFrequency
    AS ULONG       ex_CacheControl
    AS ULONG       ex_TaskID
    AS ULONG       ex_Reserved1(4)
    AS VOID PTR    ex_MMULock
    AS ULONG       ex_Reserved2(2)
    AS MinList     ex_MemHandlers
    AS VOID PTR    ex_MemHandler
END TYPE

CONST AS UINTEGER AFB_68010         = 0
CONST AS UINTEGER AFB_68020         = 1
CONST AS UINTEGER AFB_68030         = 2
CONST AS UINTEGER AFB_68040         = 3
CONST AS UINTEGER AFB_68881         = 4
CONST AS UINTEGER AFB_68882         = 5
CONST AS UINTEGER AFB_FPU40         = 6
CONST AS UINTEGER AFB_68060         = 7
CONST AS UINTEGER AFB_PRIVATE       = 15

CONST AS UINTEGER AFF_68010         = &H0001
CONST AS UINTEGER AFF_68020         = &H0002
CONST AS UINTEGER AFF_68030         = &H0004
CONST AS UINTEGER AFF_68040         = &H0008
CONST AS UINTEGER AFF_68881         = &H0010
CONST AS UINTEGER AFF_68882         = &H0020
CONST AS UINTEGER AFF_FPU40         = &H0040
CONST AS UINTEGER AFF_68060         = &H0080
CONST AS UINTEGER AFF_PRIVATE       = &H8000

CONST AS ULONG CACRF_EnableI        = 1 SHL 0
CONST AS ULONG CACRF_FreezeI        = 1 SHL 1
CONST AS ULONG CACRF_ClearI         = 1 SHL 3
CONST AS ULONG CACRF_IBE            = 1 SHL 4
CONST AS ULONG CACRF_EnableD        = 1 SHL 8
CONST AS ULONG CACRF_FreezeD        = 1 SHL 9
CONST AS ULONG CACRF_ClearD         = 1 SHL 11
CONST AS ULONG CACRF_DBE            = 1 SHL 12
CONST AS ULONG CACRF_WriteAllocate  = 1 SHL 13
CONST AS ULONG CACRF_EnableE        = 1 SHL 30
CONST AS ULONG CACRF_CopyBack       = 1 SHL 31

CONST AS UBYTE DMA_Continue         = 1 SHL 1
CONST AS UBYTE DMA_NoModify         = 1 SHL 2
CONST AS UBYTE DMA_ReadFromRAM      = 1 SHL 3

TYPE IORequest
    AS Message      io_Message
    AS Device PTR   io_Device
    AS Unit PTR     io_Unit
    AS UINTEGER     io_Command
    AS UBYTE        io_Flags
    AS BYTE         io_Error
END TYPE

TYPE IOStdReq
    AS Message      io_Message
    AS Device PTR   io_Device
    AS Unit PTR     io_Unit
    AS UINTEGER     io_Command
    AS UBYTE        io_Flags
    AS BYTE         io_Error
    AS ULONG        io_Actual
    AS ULONG        io_Length
    AS VOID PTR     io_Data
    AS ULONG        io_Offset
END TYPE

DIM AS INTEGER DEV_BEGINIO = -30
DIM AS INTEGER DEV_ABORTIO = -36

CONST AS UBYTE IOB_QUICK   = 0
CONST AS UBYTE IOF_QUICK   = 1
CONST AS UBYTE CMD_INVALID = 0
CONST AS UBYTE CMD_RESET   = 1
CONST AS UBYTE CMD_READ    = 2
CONST AS UBYTE CMD_WRITE   = 3
CONST AS UBYTE CMD_UPDATE  = 4
CONST AS UBYTE CMD_CLEAR   = 5
CONST AS UBYTE CMD_STOP    = 6
CONST AS UBYTE CMD_START   = 7
CONST AS UBYTE CMD_FLUSH   = 8
CONST AS UBYTE CMD_NONSTD  = 9

TYPE MemChunk
    AS MemChunk PTR    mc_Next
    AS ULONG           mc_Bytes
END TYPE

TYPE MemHeader
    AS Node         mh_Node
    AS UINTEGER     mh_Attributes
    AS MemChunk PTR mh_First
    AS VOID PTR     mh_Lower
    AS VOID PTR     mh_Upper
    AS ULONG        mh_Free
END TYPE

TYPE MemEntry
    AS ULONG        meu_Reqs
    AS ULONG        me_Length
END TYPE

TYPE MemList
    AS Node         ml_Node
    AS UINTEGER     ml_NumEntries
    AS MemEntry     ml_ME(0)
END TYPE

CONST AS ULONG MEMF_ANY         = 0
CONST AS ULONG MEMF_PUBLIC      = 1 SHL 0
CONST AS ULONG MEMF_CHIP        = 1 SHL 1
CONST AS ULONG MEMF_FAST        = 1 SHL 2
CONST AS ULONG MEMF_LOCAL       = 1 SHL 8
CONST AS ULONG MEMF_24BITDMA    = 1 SHL 9
CONST AS ULONG MEMF_KICK        = 1 SHL 10
CONST AS ULONG MEMF_CLEAR       = 1 SHL 16
CONST AS ULONG MEMF_LARGEST     = 1 SHL 17
CONST AS ULONG MEMF_REVERSE     = 1 SHL 18
CONST AS ULONG MEMF_TOTAL       = 1 SHL 19
CONST AS ULONG MEMF_NO_EXPUNGE  = 1 SHL 31

CONST AS ULONG MEM_BLOCKSIZE = 8
CONST AS ULONG MEM_BLOCKMASK = MEM_BLOCKSIZE-1

TYPE MemHandlerData
    AS ULONG     memh_RequestSize
    AS ULONG     memh_RequestFlags
    AS ULONG     memh_Flags
END TYPE

DIM AS LONG MEMHF_RECYCLE   = 1
DIM AS LONG MEM_DID_NOTHING = 0
DIM AS LONG MEM_ALL_DONE    = -1
DIM AS LONG MEM_TRY_AGAIN   = 1

TYPE Resident
    AS UINTEGER        rt_MatchWord
    AS Resident PTR    rt_MatchTag
    AS VOID PTR        rt_EndSkip
    AS UBYTE           rt_Flags
    AS UBYTE           rt_Version
    AS UBYTE           rt_Type
    AS BYTE            rt_Pri
    AS STRING          rt_Name
    AS STRING          rt_IdString
    AS VOID PTR        rt_Init
END TYPE

CONST AS UINTEGER RTC_MATCHWORD = &H4AFC

CONST AS UBYTE RTF_AUTOINIT     = 1 SHL 7
CONST AS UBYTE RTF_AFTERDOS     = 1 SHL 2
CONST AS UBYTE RTF_SINGLETASK   = 1 SHL 1
CONST AS UBYTE RTF_COLDSTART    = 1 SHL 0

CONST AS UBYTE RTM_WHEN         = 3

CONST AS UBYTE RTW_NEVER        = 0
CONST AS UBYTE RTW_COLDSTART    = 1

TYPE SemaphoreRequest
    AS MinNode     sr_Link
    AS Task PTR    sr_Waiter
END TYPE

TYPE SignalSemaphore
    AS Node                ss_Link
    AS INTEGER             ss_NestCount
    AS MinList             ss_WaitQueue
    AS SemaphoreRequest    ss_MultipleLink
    AS Task PTR            ss_Owner
    AS INTEGER             ss_QueueCount
END TYPE

TYPE SemaphoreMessage
    AS Message             ssm_Message
    AS SignalSemaphore PTR ssm_Semaphore
END TYPE

CONST AS ULONG SM_SHARED    = 1
CONST AS ULONG SM_EXCLUSIVE = 0

TYPE Semaphore
    AS MsgPort     sm_MsgPort
    AS INTEGER     sm_Bids
END TYPE

CONST AS UINTEGER INCLUDE_VERSION = 45

CONST AS UBYTE BYTEMASK           = &HFF
CONST AS UINTEGER LIBRARY_MINIMUM = 40

