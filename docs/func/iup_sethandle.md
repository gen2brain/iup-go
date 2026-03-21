## IupSetHandle

Associates a name with an interface element.

### Parameters/Return

    Ihandle *IupSetHandle(const char *name, Ihandle *ih);

**name**: name of the interface element.\
**ih**: identifier of the interface element. Use NULL to remove the association.

**Returns:** the identifier of the interface element previously associated to the parameter **name**.

### Notes

This function is used so it is possible to set attributes values that are in fact other elements that were created in C.
For example:

    IupSetHandle("test_image", image);
    IupSetAttribute(button, "IMAGE", "test_image");

But this code can be replaced by a more convenient function call:

    IupSetAttributeHandle(button, "IMAGE", image);

------------------------------------------------------------------------

In fact, any pointer can be stored and retrieved with **IupSetHandle** and **IupGetHandle**, not only Ihandle*.

Also, **IupSetHandle** can be called several times with the same pointer and different names.
There is no restriction for the number of names a pointer can have, but **IupGetName** will return only the last name set.

When **IupSetHandle** is called, the control will have a HANDLENAME attribute with the last name set.

### See Also

[IupGetHandle](iup_gethandle.md), [IupSetAttributeHandle](iup_setattributehandle.md)
