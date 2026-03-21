## RESIZE_CB

Action generated when the canvas or dialog size is changed.

### Callback

    int function(Ihandle *ih, int width, int height);

**ih**: identifier of the element that activated the event.\
**width**: the width of the internal element size in pixels not considering the decorations (client size)\
**height**: the height of the internal element size in pixels not considering the decorations (client size)

### Notes

For the dialog, this action is also generated when the dialog is mapped, after the map and before the show.

When XAUTOHIDE=Yes or YAUTOHIDE=Yes, if the canvas scrollbar is hidden/shown after changing the DX or DY attributes from inside the callback, the size of the drawing area will immediately change, so the parameters **with** and **height** will be invalid.
To update the parameters, consult the DRAWSIZE attribute.
Also activate the drawing toolkit only after updating the DX or DY attributes.

### Affects

[IupCanvas](../elem/iup_canvas.md), [IupGLCanvas](../ctrl/iup_glcanvas.md), [IupDialog](../dlg/iup_dialog.md)
