## IupGetCallback

Returns the callback associated to an event.

### Parameters/Return

    Icallback IupGetCallback(Ihandle* ih, const char *name);

**ih**: identifier of the interface element. **\
name**: attribute name of the callback.

**Returns:** the callback or NULL if there is none.

### Notes

This function replaces the **deprecated **combination:

    IupGetFunction(IupGetAttribute(ih, name))

If an event is associated using **IupSetFunction** and **IupSetAttribute**, the **IupGetCallback** also returns the correct callback.
So old applications work normally.

### See Also

[IupSetCallback](iup_setcallback.md),   [IupGetFunction](iup_getfunction.md)

 
