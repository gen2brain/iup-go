## IupGetAllGlobals

Returns the union of registered globals and user-set globals not present in the registry.

### Parameters/Return

    int IupGetAllGlobals(char** names, int n);

**names**: array receiving the names. Each entry points to an internal string. Pass NULL to query the count.\
**n**: maximum number of names the array can receive.

**Returns:** number of names written, or the total count if `names` is NULL or `n` is 0/-1.

### Notes

The registered set covers every well-known global across all drivers. Use [IupGetGlobalInfo](iup_getglobalinfo.md) to query per-name flags and driver support.

### See Also

[IupGetGlobalInfo](iup_getglobalinfo.md), [IupSetGlobal](iup_setglobal.md), [IupGetGlobal](iup_getglobal.md)
