## IupGetClassName

Returns the name of the class of an interface element.

### Parameters/Return

    char* IupGetClassName(Ihandle* ih);

**ih**: Identifier of the interface element.

**Returns:** the name of the class.

### Notes

The following names are known:

**Primitives:**

    "animatedlabel"
    "button"
    "calendar"
    "canvas"
    "colorbar"
    "colorbrowser"
    "datepick"
    "dial"
    "dropbutton"
    "flatbutton"
    "flatlabel"
    "flatlist"
    "flatseparator"
    "flattoggle"
    "flattree"
    "flatval"
    "gauge"
    "label"
    "link"
    "list"
    "multiline"
    "progressbar"
    "scrollbar"
    "spin"
    "table"
    "tabs"
    "flattabs"
    "text"
    "toggle"
    "tree"
    "val"

**Containers:**

    "backgroundbox"
    "cbox"
    "detachbox"
    "dialog"
    "expander"
    "fill"
    "flatframe"
    "flatscrollbox"
    "frame"
    "gridbox"
    "hbox"
    "multibox"
    "normalizer"
    "popover"
    "radio"
    "sbox"
    "scrollbox"
    "space"
    "spinbox"
    "split"
    "vbox"
    "zbox"

**Menus:**

    "item"
    "menu"
    "separator"
    "submenu"

**Images:**

    "image"
    "imagergb"
    "imagergba"

**Dialogs:**

    "colordlg"
    "filedlg"
    "fontdlg"
    "messagedlg"
    "progressdlg"
    "param"
    "parambox"

**Resources:**

    "clipboard"
    "notify"
    "thread"
    "timer"
    "tray"
    "user"

**Additional Controls** (require [ControlsOpen](iup_open.md)):

    "cells"
    "matrix"
    "matrixex"
    "matrixlist"
    "plot"

**GL Controls** (require [GLCanvasOpen](iup_open.md)):

    "glcanvas"
    "glbackgroundbox"

**Web Controls** (require [WebBrowserOpen](iup_open.md)):

    "webbrowser"

### See Also

[IupClassMatch](iup_classmatch.md), [IupGetClassType](iup_getclasstype.md), [IupGetClassAttributes](iup_getclassattributes.md)
