## SCROLL_CB

Called when some manipulation is made to the scrollbar.
The canvas is automatically redrawn only if this callback is NOT defined.

(GTK 2.8) Also, the POSX and POSY values will not be correctly updated for older GTK versions.

In Ubuntu, when liboverlay-scrollbar is enabled (the tiny auto-hide scrollbar) only the IUP_SBPOSV and IUP_SBPOSH codes are used.

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

IUP_SBDRAGH and IUP_SBDRAGV are not supported in GTK.
During drag IUP_SBPOSH and IUP_SBPOSV are used.

In Windows, after a drag when mouse is released IUP_SBPOSH or IUP_SBPOSV are called.

### Affects

[IupCanvas](../elem/iup_canvas.md), [IupGLCanvas](../ctrl/iup_glcanvas.md), [SCROLLBAR](../attrib/iup_scrollbar.md)
