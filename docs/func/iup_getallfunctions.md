## IupGetAllFunctions

Returns the well-known function-namespace names that IUP dispatches itself, plus any extra names the application has registered via [IupSetFunction](iup_setfunction.md) or [IupSetCallback](iup_setcallback.md) on the global namespace.

The well-known set covers `ENTRY_POINT`, `IDLE_ACTION`, and the global input family (`GLOBALKEYPRESS_CB`, `GLOBALBUTTON_CB`, `GLOBALMOTION_CB`, `GLOBALWHEEL_CB`). They are listed even when no callback has been bound; pair this with [IupGetFunction](iup_getfunction.md) to discover which ones are currently bound.

### Parameters/Return

    int IupGetAllFunctions(char** names, int n);

**names**: array receiving the names. Each entry points to an internal string. Pass NULL to query the count.\
**n**: maximum number of names the array can receive.

**Returns:** number of names written, or the total count if `names` is NULL or `n` is 0/-1.

Internal names (those starting with `_IUP`) are skipped.

### See Also

[IupGetFunction](iup_getfunction.md), [IupSetCallback](iup_setcallback.md)
