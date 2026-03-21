## IupGetAttributes

Returns all attributes of a given element that are set in the internal hash table.
The known attributes that are pointers (not strings) are returned as integers.

The internal attributes are not returned (attributes prefixed with "_IUP").

Before calling this function, the application must ensure that there are no pointer attributes set for that element, although all known pointers attributes are handled.

This function should be avoided. Use **IupGetAllAttributes** instead.

### Parameters/Return

    char* IupGetAttributes (Ihandle *ih);

**ih**: Identifier of the interface element.

**Returns:** a string with all attributes in the format: "NAME=VALUE, NAME="VALUE", ...".

### See Also

[IupGetAttribute](iup_getattribute.md), [IupGetAllAttributes](iup_getallattributes.md), [IupSetAttribute](iup_setattribute.md), [IupSetAttributes](iup_setattributes.md)
