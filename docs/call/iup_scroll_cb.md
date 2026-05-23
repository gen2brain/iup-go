## SCROLL_CB

Called when some manipulation is made to the scrollbar.
The canvas is automatically redrawn only if this callback is NOT defined.

### Callback

    int function(Ihandle *ih, int op, float posx, float posy);

**ih**: identifier of the element that activated the event.\
**op**: indicates the operation performed on the scrollbar.

> If the manipulation was made on the vertical scrollbar, it can have the following values:

    IUP_SBUP - line up
    IUP_SBDN - line down
    IUP_SBPGUP - page up
    IUP_SBPGDN - page down
    IUP_SBPOSV - vertical positioning
    IUP_SBDRAGV - vertical drag 

> If it was on the horizontal scrollbar, the following values are valid:

    IUP_SBLEFT - column left
    IUP_SBRIGHT - column right
    IUP_SBPGLEFT - page left
    IUP_SBPGRIGHT - page right
    IUP_SBPOSH - horizontal positioning
    IUP_SBDRAGH - horizontal drag

**posx**, **posy**: the same as the **ACTION** canvas callback (corresponding to the values of attributes POSX and POSY).

### Notes

Line and page operations are reported in Win32, Motif, GTK, Qt, WinUI and Cocoa; the drag operations IUP_SBDRAGH/IUP_SBDRAGV only in Win32 and Motif (the others use IUP_SBPOSH/IUP_SBPOSV during a drag). GTK 4, FLTK, EFL, Haiku, Android and iOS report only IUP_SBPOSH/IUP_SBPOSV. For portability prefer reading POSX and POSY.

### Affects

[IupCanvas](../elem/iup_canvas.md), [IupGLCanvas](../ctrl/iup_glcanvas.md), [IupScrollbar](../elem/iup_scrollbar.md), [SCROLLBAR](../attrib/iup_scrollbar.md)
