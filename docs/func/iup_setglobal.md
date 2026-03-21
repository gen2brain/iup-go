## IupSetGlobal

Sets an attribute in the global environment.
If the driver processes the attribute, then it will not be stored internally.

### Parameters/Return

    void IupSetGlobal(const char *name, const char *value);
    void IupSetStrGlobal(const char *name, const char *value);

**name**: name of the attribute.\
**value**: value of the attribute. If it equals NULL, the attribute will be removed.

### Notes

**IupSetGlobal** can store only constant strings (like "Title", "30", etc.) or application pointers.
The given value is not duplicated as a string, only a reference is stored.
Therefore, you can store application custom attributes, such as a context structure to be used in a callback.

**IupSetStrGlobal** (old **IupStoreGlobal**) can only store strings.
The given string value will be duplicated internally.

[IupSetAttribute](iup_setattribute.md) functions can also be used to set global attributes, just set the element to NULL.

### See Also

[IupGetGlobal](iup_getglobal.md), [Global Attributes](../attrib/iup_globals.md)
