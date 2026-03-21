## MOTION_CB

Action generated when the mouse moves.

### Callback

    int function(Ihandle *ih, int x, int y, char *status);

**ih**: identifier of the element that activated the event.\
**x**, **y**: position in the canvas where the event has occurred, in pixels.\
**status**: status of mouse buttons and certain keyboard keys at the moment the event was generated.
The same macros used for [BUTTON_CB](../call/iup_button_cb.md) can be used for this status.

### Notes

Between press and release, all mouse events are redirected only to this control, even if the cursor moves outside the element.
So the BUTTON_CB callback when released and the MOTION_CB callback can be called with coordinates outside the element rectangle.

### Affects

[IupCanvas](../elem/iup_canvas.md), [IupGLCanvas](../ctrl/iup_glcanvas.md)
