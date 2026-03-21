## IupResetAttribute

Removes an attribute from the hash table of the element, and its children if the attribute is inheritable.
It is useful to reset the state of inheritable attributes in a tree of elements.

### Parameters/Return

    void IupResetAttribute(Ihandle *ih, const char *name);

**ih**: Identifier of the interface element. If NULL will set in the global environment.\
**name**: name of the attribute.

### See Also

[IupGetAttribute](iup_getattribute.md), [IupSetAttribute](iup_setattribute.md)
