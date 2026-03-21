## SCREENPOSITION/X/Y (read-only) (non-inheritable)

Returns the absolute horizontal and/or vertical position of the top-left corner of the client area relative to the origin of the main screen in pixels.
It is similar to POSITION but relative to the origin of the main screen, instead of the origin of the client area.
The origin of the main screen is in the top-left corner, in Windows it is affected by the position of the Start Menu when it is at the top or left side of the screen.

**IMPORTANT**: For the dialog, it is the position of the top-left corner of the window, **NOT the client area**.
It is the same position used in [IupShowXY](../func/iup_showxy.md) and [IupPopup](../func/iup_popup.md).
In GTK, if the dialog is hidden the values can be outdated.

### Value

"x,y", where *x* and *y* are integer values corresponding to the horizontal and vertical position, respectively, in pixels.
When X or Y are used a single value is returned.

### Affects

All controls that have visual representation.

### See Also

[POSITION](iup_position.md)
