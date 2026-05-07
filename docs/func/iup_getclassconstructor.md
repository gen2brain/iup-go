## IupGetClassConstructor

Returns the parameter format that [IupCreate](iup_create.md) / IupCreatev / IupCreatep accepts for a class.

### Parameters/Return

    int IupGetClassConstructor(const char* classname, char** format, char** format_attr);

**classname**: name of the class.\
**format**: receives the format string, or NULL if the class has no constructor parameters. Pass NULL to ignore.\
**format_attr**: receives the attribute name that the first parameter is stored under when the format starts with `"s"` or `"a"` (typically `TITLE`). Pass NULL to ignore.

**Returns:** 1 if the class has a constructor format, 0 if the class is registered without one (`format` is NULL), -1 if the class is not registered.

### Format

Each character codes one parameter to `IupCreatev`:

| Char | C type | Notes |
|---|---|---|
| b | unsigned char | byte (unused upstream) |
| j | int* | array of integer (unused upstream) |
| f | float | real (unused upstream) |
| i | int | integer; used in IupImage* |
| c | unsigned char* | array of byte; used in IupImage* |
| s | char* | string; usually stored under `format_attr` (often `TITLE`) |
| a | char* | name of the ACTION callback; stored under `format_attr` |
| h | Ihandle* | element handle |
| g | Ihandle** | NULL-terminated array of element handles; when present there are no other parameters |

String pointers are owned by IUP.

### See Also

[IupCreate](iup_create.md), [IupGetClassInfo](iup_getclassinfo.md), [IupGetClassAttributes](iup_getclassattributes.md), [IupGetClassCallbacks](iup_getclasscallbacks.md)
