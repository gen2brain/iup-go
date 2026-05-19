## WHEEL_CB

Action generated when the mouse wheel is rotated.
If this callback is not defined, the wheel will automatically scroll the canvas in the vertical direction by some lines, the SCROLL_CB callback if defined will be called with the IUP_SBDRAGV operation.

### Callback

    int function(Ihandle *ih, float delta, int x, int y, char *status);

**ih**: identifier of the element that activated the event.\
**delta**: the amount the wheel was rotated in notches.\
**x**, **y**: position in the canvas where the event has occurred, in pixels.\
**status**: status of mouse buttons and certain keyboard keys at the moment the event was generated.
The same macros used for [BUTTON_CB](../call/iup_button_cb.md) can be used for this status.

### Notes

In GTK, GTK 4, Motif, Qt, FLTK, EFL and Haiku, delta is always 1 or -1.
In Windows and WinUI, delta is normalized in WHEEL_DELTA units (1 per notch typical, 2 or more on some devices).
In macOS, iOS and Android, delta can be fractional (precision/trackpad scrolling reports sub-notch values).

### Affects

[IupCanvas](../elem/iup_canvas.md), [IupGLCanvas](../ctrl/iup_glcanvas.md)
