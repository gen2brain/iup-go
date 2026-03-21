## IupSetCallbacks

Associates several callbacks to an event.

### Parameters/Return

    Ihandle* IupSetCallbacks(Ihandle* ih, const char *name, Icallback func, ...);
    Ihandle* IupSetCallbacksV(Ihandle* ih, const char *name, Icallback func, va_list arglist);

**ih**: identifier of the interface element. **\
name**: name of the callback.\
**func**: address of a C function. If NULL removes the association.

**Returns:** the same **ih** handle.

### Notes

It is useful for setting many callbacks at once while in the creation of an hierarchy of elements, just like **IupSetAttributes**.

### See Also

[IupSetCallback](iup_setcallback.md), [IupSetAttributes](iup_setattributes.md)
