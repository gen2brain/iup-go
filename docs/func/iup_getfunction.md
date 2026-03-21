## IupGetFunction

Returns the function associated to an action only when they were set by [IupSetFunction](iup_setfunction.md).
It will not work if [IupSetCallback](iup_setcallback.md) were used.

### Parameters/Return

    Icallback IupGetFunction(const char *name);

**name**: name of the action.

**Returns**: the callback or NULL if not found.

### See Also

[IupSetFunction](iup_setfunction.md),  [IupGetCallback](iup_getcallback.md)

 
