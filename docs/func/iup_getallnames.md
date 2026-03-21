## IupGetAllNames

Returns the names of all interface elements that have an associated name using **IupSetHandle**.

### Parameters/Return

    int IupGetAllNames(char** names, int max_n);

**names**: table receiving the names. Only the list of names needs to be allocated.
Each name will point to an internal string.\
**max_n**: maximum number of names the table can receive.

**Returns:** the number of names loaded to the table.
If names==NULL or max_n==0 or -1 then returns the actual number of names.

### See Also

[IupSetHandle](iup_sethandle.md), [IupGetHandle](iup_gethandle.md), [IupGetName](iup_getname.md), [IupGetAllDialogs](iup_getalldialogs.md).
