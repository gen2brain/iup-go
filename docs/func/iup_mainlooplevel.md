## IupMainLoopLevel

Returns the current cascade level of **IupMainLoop**. When no calls were done, return value is 0.

### Parameters/Return

    int IupMainLoopLevel(void);

**Returns:** the cascade level.

### Notes

Can be used to check if **IupMainLoop** was already called and avoid calling it again.

A call to **IupPopup** will increase one level.

### See Also

[IupOpen](iup_open.md), [IupClose](iup_close.md), [IupLoopStep](iup_loopstep.md), [IDLE_ACTION](../call/iup_idle_action.md), [LOCKLOOP](../attrib/iup_globals.md#lockloop).
