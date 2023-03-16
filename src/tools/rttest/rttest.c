struct ExecBase      *SysBase       = NULL;
struct DOSBase       *DOSBase       = NULL;
struct MathBase      *MathBase      = NULL;
struct MathTransBase *MathTransBase = NULL;
struct UtilityBase   *UtilityBase   = NULL;

void _cstartup (void)
{
    SysBase = (*((struct ExecBase **) 4));

    if (!(DOSBase = (struct DOSBase *)OpenLibrary((CONST_STRPTR) "dos.library", 37)))
        _cshutdown(20, (UBYTE *) "*** error: failed to open dos.library!\n");

    _debug_stdout = Output();

    if (!(MathBase = (struct MathBase *)OpenLibrary((CONST_STRPTR) "mathffp.library", 37)))
        _cshutdown(20, (UBYTE *) "*** error: failed to open mathffp.library!\n");

    if (!(MathTransBase = (struct MathTransBase *)OpenLibrary((CONST_STRPTR) "mathtrans.library", 37)))
        _cshutdown(20, (UBYTE *) "*** error: failed to open mathtrans.library!\n");

    if (!(UtilityBase = (struct UtilityBase *)OpenLibrary((CONST_STRPTR) "utility.library", 37)))
        _cshutdown(20, (UBYTE *) "*** error: failed to open utility.library!\n");

    DPRINTF ("_cstartup: _aqb_stack_size=%d\n", _aqb_stack_size);
    if (_aqb_stack_size)
    {
        _g_stack = AllocVec (_aqb_stack_size, MEMF_PUBLIC);
        if (!_g_stack)
            _cshutdown(20, (UBYTE *) "*** error: failed to allocate stack!\n");

        DPRINTF ("_cstartup: allocated custom stack: 0x%08lx\n", _g_stack);
    }

    _autil_init();
    autil_init_done = TRUE;

    // detect startup mode

    if (!__StartupMsg)
    {
        _startup_mode = STARTUP_CLI;
    }
    else
    {
		if (__StartupMsg->debug_sig == DEBUG_SIG)
		{
			_startup_mode = STARTUP_DEBUG;
			if ( !(g_dbgPort=_autil_create_port(NULL, 0)) )
				_cshutdown(20, (UBYTE *) "*** error: failed to allocate debug port!\n");
		}
		else
		{
			_startup_mode = STARTUP_WBENCH;
		}
    }

    /* set up break signal exception + handler */

    _autil_task = FindTask(NULL);

    Forbid();
    _autil_task->tc_ExceptData = NULL;
    _autil_task->tc_ExceptCode = ___breakHandler;
    SetSignal (0, SIGBREAKF_CTRL_C);
    SetExcept (SIGBREAKF_CTRL_C, SIGBREAKF_CTRL_C);
    Permit();

    /* install CTRL+C input event handler */

	if ( !(g_inputPort=_autil_create_port(NULL, 0)) )
        _cshutdown(20, (UBYTE *) "*** error: failed to allocate CTRL-C handler input port!\n");

    if ( !(g_inputHandler=AllocMem(sizeof(struct Interrupt), MEMF_PUBLIC|MEMF_CLEAR)) )
        _cshutdown(20, (UBYTE *) "*** error: failed to allocate CTRL-C handler memory!\n");

    if ( !(g_inputReqBlk=(struct IOStdReq *)_autil_create_ext_io(g_inputPort, sizeof(struct IOStdReq))) )
        _cshutdown(20, (UBYTE *) "*** error: failed to allocate CTRL-C ext io!\n");

    if (OpenDevice ((STRPTR)"input.device", /*unitNumber=*/0, (struct IORequest *)g_inputReqBlk, /*flags=*/0))
        _cshutdown(20, (UBYTE *) "*** error: failed to open input.device!\n");
    g_inputDeviceOpen = TRUE;

    g_inputHandler->is_Code         = (APTR) ___inputHandler;
    g_inputHandler->is_Data         = (APTR) _autil_task;
    g_inputHandler->is_Node.ln_Pri  = 100;
    g_inputHandler->is_Node.ln_Name = g_inputHandlerName;

    g_inputReqBlk->io_Data    = (APTR)g_inputHandler;
    g_inputReqBlk->io_Command = IND_ADDHANDLER;

    DoIO((struct IORequest *)g_inputReqBlk);
    g_InputHandlerInstalled = TRUE;

    _astr_init();

    _amath_init();

    _aio_init();
    aio_init_done = TRUE;
}

int main(void)
{

    return 0;
}

