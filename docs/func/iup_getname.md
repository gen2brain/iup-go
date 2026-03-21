## IupGetName

Returns a name of an interface element if the element has an associated name using **IupSetHandle**.

Notice that a handle can have many names. **IupGetName** will return the last name set.

### Parameters/Return

    char* IupGetName(Ihandle* ih);

**ih**: Identifier of the interface element.

**Returns:** the name.

### See Also

[IupSetHandle](iup_sethandle.md), [IupGetHandle](iup_gethandle.md), [IupGetAllNames](iup_getallnames.md).
