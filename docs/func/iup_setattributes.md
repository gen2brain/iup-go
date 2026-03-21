## IupSetAttributes

Sets several attributes of an interface element.

### Parameters/Return

    Ihandle *IupSetAttributes(Ihandle *ih, const char *str);

**ih**: Identifier of the interface element.\
**str**: string with the attributes in the format "v1=a1, v2=a2,..." where vi is the name of an attribute and ai is its value.

**Returns**: the same **ih**.

### Examples

This function returns the same Ihandle it receives. This way, it is a lot easier to create dialogs in C.
See also [IupSetCallbacks](iup_setcallbacks.md).

    dialog = IupSetAttributes(IupDialog(
        IupSetAttributes(IupHBox(
           canvas = IupSetAttributes(IupCanvas(NULL), "BORDER=NO, RASTERSIZE=100x100"),
           NULL), "MARGIN=10x10"),
        "TITLE=Test");

Creates a list with country names and defines Japan as the selected option.

    Ihandle *list = IupList (NULL);
    IupSetAttributes(list,"VALUE=3,1=Brazil,2=USA,3=Japan,4=France");

To set values that have spaces or that may interfere with the string parse, use double quotes around the value with the backslash for C syntax.
For instance:

    IupSetAttributes(list,"1=Brazil,2=\"United States\",3=Japan,4=\"Dominican Republic\"");

### See Also

[IupGetAttribute](iup_getattribute.md), [IupSetAttribute](iup_setattribute.md), [IupGetAttributes](iup_getattributes.md), [IupSetAtt](iup_setatt.md)
