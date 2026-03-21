## IupSetClassDefaultAttribute

Changes the default value of an attribute for a class.
It can be any attribute, i.e., registered attributes or user custom attributes.

### Parameters/Return

    void IupSetClassDefaultAttribute(const char* classname, const char *name, const char *value);

**classname**: name of the class **\
name**: name of the attribute\
**value**: new default value.

### Notes

If the value is DEFAULTFONT, DLGBGCOLOR, DLGFGCOLOR, TXTBGCOLOR, TXTFGCOLOR, LINKFGCOLOR or MENUBGCOLOR, then the actual default value will be the global attribute of the same name consulted at the time the attribute is consulted.

Attributes that are not strings and attributes that have variable names, like those which has a complementary number, cannot have a default value.
Some attributes cannot have a default value by definition.

If the new default value is (char*)-1, then the default value is set to be the system default if any is defined.

### See Also

[IupGetClassName](iup_getclassname.md), [IupGetClassType](iup_getclasstype.md), [IupGetAllAttributes](iup_getallattributes.md)
