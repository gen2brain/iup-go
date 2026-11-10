## IupGetAllClasses

Returns the names of all registered classes.

### Parameters/Return

    int IupGetAllClasses(char** names, int max_n);

**names**: array receiving the names. Only the list of names needs to be allocated.
Each name will point to an internal string.\
**max_n**: maximum number of names the array can receive.

**Returns:** the actual number of names loaded to the array.
If names==NULL or max_n==0 or -1, returns the total number of names.

### See Also

[IupGetClassName](iup_getclassname.md), [IupGetClassType](iup_getclasstype.md), [IupGetAllAttributes](iup_getallattributes.md)
