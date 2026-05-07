## IupGetClassAttributeInfo

Returns the registered metadata of an attribute on a class: default value, system default, and flag bits.

### Parameters/Return

    int IupGetClassAttributeInfo(const char* classname, const char* name,
                                 char** default_value, char** system_default, int* flags);

**classname**: name of the class.\
**name**: attribute name.\
**default_value**: registered default, internal string. NULL to ignore.\
**system_default**: value used when the default was registered with the IUPAF_SAMEASSYSTEM sentinel. NULL to ignore.\
**flags**: bitwise OR of the IUPAF_* flags below. NULL to ignore.

**Returns:** 1 if found, 0 if the class exists but the attribute is not registered (output values are zero-filled), -1 if the class is not registered.

### Flags

| Flag | Value | Meaning |
|---|---|---|
| IUPAF_DEFAULT | 0 | inheritable string with default, set/get only when mapped, no id |
| IUPAF_NO_INHERIT | 1 | not inheritable |
| IUPAF_NO_DEFAULTVALUE | 2 | cannot have a default value |
| IUPAF_NO_STRING | 4 | not a string (RGB, integer, Ihandle*, ...) |
| IUPAF_NOT_MAPPED | 8 | set/get also called when not mapped |
| IUPAF_HAS_ID | 16 | one numeric id appended to the name |
| IUPAF_READONLY | 32 | read only |
| IUPAF_WRITEONLY | 64 | write only (usually an action) |
| IUPAF_HAS_ID2 | 128 | two numeric ids appended (line, column) |
| IUPAF_CALLBACK | 256 | entry is a callback, not an attribute |
| IUPAF_NO_SAVE | 512 | not directly savable by IupSaveClassAttributes |
| IUPAF_NOT_SUPPORTED | 1024 | declared but not supported by this driver |
| IUPAF_IHANDLENAME | 2048 | value is a name registered via IupSetHandle |
| IUPAF_IHANDLE | 4096 | value is an Ihandle* |

String pointers are owned by IUP. Callbacks share the table with attributes; use [IupGetClassCallbackFormat](iup_getclasscallbackformat.md) to read their signature.

### See Also

[IupGetClassAttributes](iup_getclassattributes.md), [IupGetClassCallbacks](iup_getclasscallbacks.md), [IupGetClassCallbackFormat](iup_getclasscallbackformat.md), [IupGetClassInfo](iup_getclassinfo.md)
