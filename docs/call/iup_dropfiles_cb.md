## DROPFILES_CB

Action called when a file is "dropped" into the control.
When several files are dropped at once, the callback is called several times, once for each file.

If defined after the element is mapped then the attribute DROPFILESTARGET must be set to YES.

In Motif, file drops from external applications are received via the XDND protocol.

### Callback

    int function(Ihandle *ih, const char* filename, int num, int x, int y);

**ih**: identifier of the element that activated the event.\
**filename**: Name of the dropped file.\
**num**: Number index of the dropped file.
If several files are dropped, **num** is the index of the dropped file starting from "total-1" to "0".\
**x**: X coordinate of the point where the user released the mouse button.\
**y**: Y coordinate of the point where the user released the mouse button.

**Returns**: If IUP_IGNORE is returned the callback will NOT be called for the next dropped files, and the processing of dropped files will be interrupted.

### Affects

[IupDialog](../dlg/iup_dialog.md), [IupCanvas](../elem/iup_canvas.md), [IupGLCanvas](../ctrl/iup_glcanvas.md), [IupText](../elem/iup_text.md), [IupList](../elem/iup_list.md)
