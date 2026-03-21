## IupGetClassType

Returns the name of the native type of an interface element.

### Parameters/Return

    char* IupGetClassType(Ihandle* ih);

**ih**: Identifier of the interface element.

**Returns**: the class type.

### Notes

There are only a few pre-defined class types:

    "void" - No native representation - HBOX, VBOX, ZBOX, FILL, RADIO, ...
    "control" - Native controls - BUTTON, LABEL, TOGGLE, LIST, TEXT, MULTILINE, FRAME, ...
    "canvas" - Drawing canvas, also used as a base control for custom controls (Flat* elements, GL*, Plot, Matrix, ...)
    "dialog" - dialogs, pre-defined dialogs
    "image" - all image types
    "menu" - all menu types: MENU, SUBMENU, ITEM, SEPARATOR
    "other" - other resources: TIMER, CLIPBOARD, USER, ...

### See Also

[IupGetClassName](iup_getclassname.md), [IupGetClassAttributes](iup_getclassattributes.md)
