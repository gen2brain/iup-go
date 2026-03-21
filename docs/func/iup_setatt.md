## IupSetAtt

Sets several attributes of an interface element and optionally sets its name.

### Parameters/Return

    Ihandle* IupSetAtt(const char* handle_name, Ihandle* ih, const char* name, ...);
    Ihandle* IupSetAttV(const char* handle_name, Ihandle* ih, const char* name, va_list arglist);

**handle_name:** optional handle name. **IupSetHandle** will be called internally. can be NULL**.\
ih**: Identifier of the interface element.\
**name**: name of the first attribute.\
...: after **name** a **value** must be set, then a sequence of name and value pairs can follow until a NULL name is found.
It must be a constant string because **IupSetAttribute** will be used internally.

**Returns: ih**

### Examples

This function returns the same Ihandle it receives. This way, it is a lot easier to create dialogs in C.
See also [IupSetCallbacks](iup_setcallbacks.md).

    dialog = IupSetAtt("MainDialog", IupDialog(
        IupSetAtt(NULL, IupHBox(
           IupSetAtt("MainCanvas", IupCanvas(NULL), "BORDER", "NO", "RASTERSIZE", "100x100", NULL),
           NULL), "MARGIN", "10x10", NULL),
        "TITLE", "Test", NULL);

Creates a list with country names and defines Japan as the selected option.

    Ihandle *list = IupList(NULL);
    IupSetAtt(NULL, list, "VALUE", "3", "1", "Brazil", "2", "USA", "3", "Japan", "4", "France", NULL);

### See Also

[IupGetAttribute](iup_getattribute.md), [IupSetAttribute](iup_setattribute.md), [IupGetAttributes](iup_getattributes.md), [IupSetAttributes](iup_setattributes.md)
