## IupGetAttributeHandle

Instead of using IupGetAttribute and IupGetHandle, this function directly returns the associated handle.

### Parameters/Return

    Ihandle* IupGetAttributeHandle(Ihandle *ih, const char *name);
    Ihandle* IupGetAttributeHandleId(Ihandle *ih, const char *name, int id);
    Ihandle* IupGetAttributeHandleId2(Ihandle *ih, const char *name, int lin, int col);

**ih**: identifier of the interface element.\
**name**: name of the attribute.\
**id, lin, col**: used when the attribute has additional ids.

**Returns:** the element with the associated name.
The function will not check for inheritance since all the attributes that associate handles are not inheritable.

### See Also

[IupSetAttributeHandle](iup_setattributehandle.md), [IupSetAttribute](iup_setattribute.md), [IupSetAttributes](iup_setattributes.md), [IupSetHandle](iup_sethandle.md)
