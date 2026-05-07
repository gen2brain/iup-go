## IupGetClassCallbackFormat

Returns the parameter format string of a registered callback, or NULL if the class or callback is not registered. The pointer is owned by IUP.

### Parameters/Return

    char* IupGetClassCallbackFormat(const char* classname, const char* name);

**classname**: name of the class.\
**name**: callback name (e.g. `"ACTION"`, `"BUTTON_CB"`).

### Format

Each character codes one parameter after the implicit `Ihandle* ih`. Default return type is int; a different return is appended after `=`.

| Char | C type |
|---|---|
| c | unsigned char |
| i | int |
| I | int* (array or out-parameter) |
| f | float |
| d | double |
| s | char* |
| V | void* |
| n | Ihandle* |

Examples: `""` is `int(Ihandle*)`, `"iis"` is `int(Ihandle*, int, int, char*)`, `"i=s"` is `char*(Ihandle*, int)`.

A NULL format means the default `Icallback` signature: `int callback(Ihandle*)`.

### See Also

[IupGetClassCallbacks](iup_getclasscallbacks.md), [IupGetClassAttributeInfo](iup_getclassattributeinfo.md), [IupGetClassInfo](iup_getclassinfo.md)
