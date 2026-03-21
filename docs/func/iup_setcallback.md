## IupSetCallback

Associates a callback to an event.

### Parameters/Return

    Icallback IupSetCallback(Ihandle* ih, const char *name, Icallback func);

**ih**: identifier of the interface element. **\
name**: name of the callback.\
**func**: address of a C function. If NULL removes the association.

**Returns:** the address of the previous function associated to the action.

### Notes

This function replaces the **deprecated **combination:

    IupSetFunction(global_name, func);
    IupSetAttribute(ih, name, global_name);

So it eliminates the need for a global name.

Callbacks set using **IupSetCallback** cannot be retrieved using **IupGetFunction**.

### See Also

[IupGetCallback](iup_getcallback.md), [IupSetFunction](iup_setfunction.md)
