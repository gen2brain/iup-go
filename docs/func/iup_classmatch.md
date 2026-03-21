## IupClassMatch

Checks if the give class name matches the class name of the given interface element.

### Parameters/Return

    int IupClassMatch(Ihandle* ih, const char* classname);

**ih**: Identifier of the interface element.\
**classname**: name of the class to match.

**Returns**: true (1) if the given name matches the class name or one of its parent class names, false (0) or else.

### See Also

[IupGetClassName](iup_getclassname.md)
