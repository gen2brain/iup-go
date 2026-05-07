## IupGetClassInfo

Returns class-level metadata: parent class, native type, child constraints, focus interactivity and id-attribute support.

### Parameters/Return

    int IupGetClassInfo(const char* classname,
                        char** parent, char** native_type,
                        int* child_type, int* is_interactive, int* has_attrib_id);

**classname**: name of the class.\
**parent**: parent class name as an internal string, or NULL when the class has no parent. NULL to ignore.\
**native_type**: one of `"void"`, `"control"`, `"canvas"`, `"dialog"`, `"image"`, `"menu"`, `"other"` (same set as [IupGetClassType](iup_getclasstype.md)). NULL to ignore.\
**child_type**: 0 = no children, -1 = unlimited, n>0 = fixed maximum. NULL to ignore.\
**is_interactive**: 1 if the class can take keyboard focus. NULL to ignore.\
**has_attrib_id**: 0, 1 or 2; non-zero means some attributes accept a numeric id suffix. 2 means two ids (line, column). NULL to ignore.

**Returns:** 0 on success, -1 if the class is not registered.

String pointers are owned by IUP.

### See Also

[IupGetAllClasses](iup_getallclasses.md), [IupGetClassName](iup_getclassname.md), [IupGetClassType](iup_getclasstype.md), [IupGetClassAttributeInfo](iup_getclassattributeinfo.md), [IupGetClassCallbackFormat](iup_getclasscallbackformat.md)
