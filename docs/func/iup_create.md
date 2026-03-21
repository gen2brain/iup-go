## IupCreate

Creates an interface element given its class name and parameters.
This function is called from all constructors like IupDialog(...), IupLabel(...), and so on.

After **creation** the element still needs to be **attached** to a container and **mapped** to the native system so it can be visible.

### Parameters/Return

    Ihandle* IupCreate(const char *classname);
    Ihandle* IupCreatev(const char *classname, void **params);
    Ihandle *IupCreatep(const char *classname, void* params0, ...);
    Ihandle* IupCreateV(const char *classname, void* first, va_list arglist);

**classname**: class name of the element to be created\
**params**: list of parameters limited by a NULL.

**Returns:** the identifier of the created element, or NULL if an error occurs.

### See Also

[IupAppend](iup_append.md), [IupDetach](iup_detach.md), [IupMap](iup_map.md), [IupUnmap](iup_unmap.md), [IupDestroy](iup_destroy.md), [IupGetClassName](iup_getclassname.md)
