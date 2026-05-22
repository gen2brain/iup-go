## IupSetCallback

Associates a callback to an event.

### Parameters/Return

    Icallback IupSetCallback(Ihandle* ih, const char *name, Icallback func);

**ih**: identifier of the interface element. **\
name**: name of the callback.\
**func**: address of a C function. If NULL removes the association.

**Returns:** the address of the previous function associated to the action.

### See Also

[IupGetCallback](iup_getcallback.md)
