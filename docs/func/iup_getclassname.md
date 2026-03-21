## IupGetClassName (renamed from IupGetType in 2.7)

Returns the name of the class of an interface element.

### Parameters/Return

    char* IupGetClassName(Ihandle* ih);

**ih**: Identifier of the interface element.

**Returns:** the name of the class.

### Notes

The following names are known:

    "image"
    "button"
    "canvas"
    "dialog"
    "fill"
    "frame" 
    "hbox"
    "item"
    "separator"
    "submenu"
    "label"
    "list"
    "menu"
    "radio"
    "text" 
    "toggle"
    "vbox"
    "zbox"
    "multiline"
    "user"
    "matrix"
    "tree"
    "dial"
    "gauge"
    "val"
    "glcanvas"
    "tabs"
    "cells"
    "colorbrowser"
    "colorbar"
    "spin"
    "sbox"
    "cbox"
    "progressbar"
    "olecontrol"

### See Also

[IupClassMatch](iup_classmatch.md), [IupGetClassType](iup_getclasstype.md), [IupGetClassAttributes](iup_getclassattributes.md)
