## POSX

Thumbnail position in the horizontal scrollbar in any unit.

### Value

Any floating-point value. Must be a value between XMIN and XMAX-DX.

Default: "0.0"

### Notes

When the canvas is visible, a change in POSX can generate a redraw in the horizontal scrollbar on the screen, but will NOT generate a redraw of the canvas.

### Affects

[IupCanvas](../elem/iup_canvas.md)

### See Also

[SCROLLBAR](iup_scrollbar.md)
