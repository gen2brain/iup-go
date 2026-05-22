## IupSetFunction

Associates a function to a global callback name, such as IDLE_ACTION, ENTRY_POINT, and the GLOBAL*_CB family.

### Parameters/Return

    Icallback IupSetFunction(const char *name, Icallback func);

**name**: name of the global callback.\
**func**: address of a C function. If NULL removes the association.

**Returns:** the address of the previous function associated to the name.

### See Also

[IupGetFunction](iup_getfunction.md)
