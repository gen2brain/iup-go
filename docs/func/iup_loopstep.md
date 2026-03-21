## IupLoopStep

Runs one iteration of the message loop.

### Parameters/Return

    int IupLoopStep(void);
    int IupLoopStepWait(void);


**Returns:** IUP_CLOSE or IUP_DEFAULT.

### Notes

This function is useful for allowing a second message loop to be managed by the application itself.
This means that messages can be intercepted and callbacks can be processed inside an application loop.

**IupLoopStep** returns immediately after processing any messages, or if there are no messages to process.
**IupLoopStepWait** put the system in idle until a message is processed.

If IUP_CLOSE is returned, the **IupMainLoop** will not end because the return code was already processed.
If you want to end **IupMainLoop** when IUP_CLOSE is returned by **IupLoopStep** then call **IupExitLoop** after **IupLoopStep** returns.

An example of how to use this function is a counter that can be stopped by the user.
For such, the user has to interact with the system, which is possible by calling the function periodically.

This way, this function replaces old mechanisms implemented using the Idle callback.

Note that this function does not replace **IupMainLoop**.

### See Also

[IupOpen](iup_open.md), [IupClose](iup_close.md), [IupMainLoop](iup_mainloop.md), [IupExitLoop](iup_exitloop.md), [IDLE_ACTION](../call/iup_idle_action.md)
