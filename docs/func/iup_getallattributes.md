## IupGetAllAttributes

Returns the names of all attributes of an element that are set in its internal hash table only.

### Parameters/Return

    int IupGetAllAttributes(Ihandle* ih, char** names, int max_n);

**ih**: identifier of the interface element. **\
names**: table receiving the names. Only the list of names needs to be allocated.
Each name will point to an internal string.\
**max_n**: maximum number of names the table can receive.

**Returns:** the actual number of names loaded to the table.
If names==NULL or max_n==0 or -1 then returns the maximum number of names.

### See Also

[IupGetAttribute](iup_getattribute.md), [IupSetAttribute](iup_setattribute.md), [IupSetAttributes](iup_setattributes.md)
