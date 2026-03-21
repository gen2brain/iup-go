## POSY

Thumbnail position in the vertical scrollbar in any unit.

### Value

Any floating-point value. Must be a value between YMIN and YMAX-DY.

Default: "0.0"

### Notes

When the canvas is visible, a change in POSY can generate a redraw in the vertical scrollbar on the screen, but will NOT generate a redraw of the canvas.

### Affects

[IupCanvas](../elem/iup_canvas.md)

### See Also

[SCROLLBAR](iup_scrollbar.md)
