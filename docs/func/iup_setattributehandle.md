## IupSetAttributeHandle

Instead of using **IupSetHandle** and **IupSetAttribute** with a new name, this function automatically creates a non-conflicting name and associates the name with the attribute.

It is useful for associating images and menus.

### Parameters/Return

    void IupSetAttributeHandle(Ihandle *ih, const char *name, Ihandle *ih_named);
    void IupSetAttributeHandleId(Ihandle *ih, const char *name, int id, Ihandle *ih_named);
    void IupSetAttributeHandleId2(Ihandle *ih, const char *name, int lin, int col, Ihandle *ih_named);

**ih**: identifier of the interface element.\
**name**: name of the attribute.\
**id, lin, col**: used when the attribute has additional ids.\
**ih_named**: element to associate using a name

The function will not check for inheritance since all the attributes that associate handles are not inheritable.

The attribute receives the name, so use it with attributes that take a handle name, such as IMAGE or the VALUE of an IupZbox.
Attributes that take the handle itself, such as VALUE_HANDLE, are set with **IupSetAttribute** and the handle: `IupSetAttribute(zbox, "VALUE_HANDLE", (char*)child)`.

### See Also

[IupGetAttributeHandle](iup_getattributehandle.md), [IupSetAttribute](iup_setattribute.md), [IupSetAttributes](iup_setattributes.md), [IupSetHandle](iup_sethandle.md)
