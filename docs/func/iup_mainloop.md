## IupMainLoop

Executes the user interaction until a callback returns IUP_CLOSE, **IupExitLoop** is called, or hiding the last visible dialog.

### Parameters/Return

    int IupMainLoop(void);

**Returns:** IUP_NOERROR always.

### Notes

When this function is called, it will interrupt the program execution until a callback returns IUP_CLOSE, **IupExitLoop** is called, or there are no visible dialogs.

If you cascade many calls to **IupMainLoop** there must be a "return IUP_CLOSE" or **IupExitLoop** call for each cascade level, hiding all dialogs will close only one level.
Call [IupMainLoopLevel](iup_mainlooplevel.md) to obtain the current level.

If **IupMainLoop** is called without any visible dialogs and no active timers, the application will hang and will not be possible to close the main loop.
The process will have to be interrupted by the system.

When the last visible dialog is hidden, the **IupExitLoop** function is automatically called, causing the **IupMainLoop** to return.
To avoid that, set LOCKLOOP=YES before hiding the last dialog.

On Android and iOS the host platform owns the event loop. IupMainLoop returns immediately on Android and never returns on iOS, so cleanup placed after it cannot be relied upon, use [CLOSE_CB](../call/iup_close_cb.md) instead. IupClose is a no-op on both.

### See Also

[IupOpen](iup_open.md), [IupClose](iup_close.md), [IupLoopStep](iup_loopstep.md), [IupExitLoop](iup_exitloop.md), [IDLE_ACTION](../call/iup_idle_action.md), [LOCKLOOP](../attrib/iup_globals.md#lockloop).
