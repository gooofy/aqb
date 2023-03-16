
extern struct DebugMsg  *__StartupMsg;

void _debug_putc(const char c)
{
    if ((_startup_mode == STARTUP_DEBUG) && g_dbgPort)
	{
        _dbgOutputMsg.msg.mn_Node.ln_Succ = NULL;
        _dbgOutputMsg.msg.mn_Node.ln_Pred = NULL;
        _dbgOutputMsg.msg.mn_Node.ln_Pri  = 0;
        _dbgOutputMsg.msg.mn_Node.ln_Name = NULL;
		_dbgOutputMsg.msg.mn_Node.ln_Type = NT_MESSAGE;
		_dbgOutputMsg.msg.mn_Length       = sizeof(struct DebugMsg);
		_dbgOutputMsg.msg.mn_ReplyPort    = g_dbgPort;
		_dbgOutputMsg.debug_sig           = DEBUG_SIG;
		_dbgOutputMsg.debug_cmd           = DEBUG_CMD_PUTC;
		_dbgOutputMsg.u.c                 = c;

		PutMsg (__StartupMsg->msg.mn_ReplyPort, &_dbgOutputMsg.msg);
		WaitPort(g_dbgPort);
        GetMsg(g_dbgPort); // discard reply
	}
    else
    {
        if (_debug_stdout)
            Write(_debug_stdout, (CONST APTR) &c, 1);
    }
}

void _debug_puts(const UBYTE *s)
{
    if ((_startup_mode == STARTUP_DEBUG) && g_dbgPort)
	{
        _dbgOutputMsg.msg.mn_Node.ln_Succ = NULL;
        _dbgOutputMsg.msg.mn_Node.ln_Pred = NULL;
        _dbgOutputMsg.msg.mn_Node.ln_Pri  = 0;
        _dbgOutputMsg.msg.mn_Node.ln_Name = NULL;
		_dbgOutputMsg.msg.mn_Node.ln_Type = NT_MESSAGE;
		_dbgOutputMsg.msg.mn_Length       = sizeof(struct DebugMsg);
		_dbgOutputMsg.msg.mn_ReplyPort    = g_dbgPort;
		_dbgOutputMsg.debug_sig           = DEBUG_SIG;
		_dbgOutputMsg.debug_cmd           = DEBUG_CMD_PUTS;
		_dbgOutputMsg.u.str               = (char *) s;

        // Write(_debug_stdout, (STRPTR) "_debug_puts:2PutMsg\n", 20);
		PutMsg (__StartupMsg->msg.mn_ReplyPort, &_dbgOutputMsg.msg);
		WaitPort(g_dbgPort);
        GetMsg(g_dbgPort); // discard reply
    }
    else
    {
		if (_debug_stdout)
			Write(_debug_stdout, (CONST APTR) s, LEN_(s));
	}
}

void _debugger_init(void)
{
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
}
