## IupGetClassType

Returns the name of the native type of an interface element.

### Parameters/Return

    char* IupGetClassType(Ihandle* ih);

**ih**: Identifier of the interface element.

**Returns**: the class type.

### Notes

There are only a few pre-defined class types:

    "void" - No native representation - HBOX, VBOX, ZBOX, FILL, RADIO, NORMALIZER, ...
    "control" - Native controls - BUTTON, LABEL, TOGGLE, LIST, TEXT, MULTILINE, FRAME, TABS, TABLE, SCROLLBAR, ...
    "canvas" - Drawing canvas, also used as a base control for custom controls (Flat* elements, GL*, Plot, Matrix, ...)
    "dialog" - Dialogs and pre-defined dialogs
    "image" - All image types: IMAGE, IMAGERGB, IMAGERGBA
    "menu" - All menu types: MENU, SUBMENU, ITEM, SEPARATOR
    "other" - Other resources: TIMER, CLIPBOARD, USER, THREAD, TRAY, NOTIFY, ...

### See Also

[IupGetClassName](iup_getclassname.md), [IupGetClassAttributes](iup_getclassattributes.md)
