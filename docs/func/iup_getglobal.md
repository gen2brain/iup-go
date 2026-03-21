## IupGetGlobal

Returns an attribute value from the global environment.
The value can be returned from the driver or from the internal storage.

### Parameters/Return

    char *IupGetGlobal(const char *name);

**name**: name of the attribute.

**Returns:** the attribute value. If the attribute does not exist, NULL is returned.

### Notes

This function’s return value is not necessarily the same one used by the application to set the attribute’s value.

The returned value is not necessarily the same pointer used by the application to define the attribute value.
The pointers of internal IUP attributes returned by **IupGetGlobal** should **never** be freed or changed, except when it is a custom application pointer that was stored using **IupSetGlobal** and allocated by the application.

 The returned pointer can be used safely even if **IupGetGlobal** or **IupGetAttribute** are called several times.
But not too many times, because it is an internal buffer and after IUP may reuse it after around 50 calls.

[IupGetAttribute](iup_getattribute.md) can also be used to get global attributes, just set the element to NULL.

### See Also

 [IupSetGlobal](iup_setglobal.md), [Global Attributes](../attrib/iup_globals.md)
