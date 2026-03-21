## IupSetFunction

Associates a function to an action as a global callback.

**This function should not be used by new applications, use it only for global callbacks.** For regular elements use [IupSetCallback](iup_setcallback.md) instead.

Notice that the application or libraries may set the same name for two different functions by mistake.
**IupSetCallback** does not depend on global names.

### Parameters/Return

    Icallback IupSetFunction(const char *name, Icallback func);

**name**: name of an action.\
**func**: address of a C function. If NULL removes the association.

**Returns:** the address of the previous function associated to the action.

### See Also

[IupGetFunction](iup_getfunction.md), [IupSetCallback](iup_setcallback.md),  
