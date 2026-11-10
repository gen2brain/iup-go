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
    "label"
    "link"
    "list"
    "multiline"
    "progressbar"
    "scrollbar"
    "separator"
    "spin"
    "table"
    "tabs"
    "terminal"
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

    "menu"
    "menuitem"
    "menuseparator"
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
    "printdlg"
    "progressdlg"
    "param"
    "parambox"
    "sharedlg"

**Resources:**

    "clipboard"
    "drawpath"
    "fdwatch"
    "location"
    "notify"
    "sensor"
    "thread"
    "timer"
    "tray"
    "user"

**Additional Controls** (require [ControlsOpen](iup_open.md)):

    "cells"
    "dropbutton"
    "flatbutton"
    "flatframe"
    "flatlabel"
    "flatlist"
    "flatscrollbox"
    "flattabs"
    "flattoggle"
    "flattree"
    "flatval"
    "gauge"
    "matrix"
    "matrixex"
    "matrixlist"

**GL Controls** (require [GLCanvasOpen](iup_open.md)):

    "glcanvas"
    "glbackgroundbox"

**Plot** (requires [IupPlotOpen](../ctrl/iup_plot.md)):

    "plot"

**Media** (require [IupMediaOpen](../ctrl/iup_audio.md)):

    "audio"
    "camera"
    "microphone"
    "video"

**Web Controls** (require [WebBrowserOpen](iup_open.md)):

    "webbrowser"

### See Also

[IupClassMatch](iup_classmatch.md), [IupGetClassType](iup_getclasstype.md), [IupGetClassAttributes](iup_getclassattributes.md)
