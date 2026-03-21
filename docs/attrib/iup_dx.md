## DX

Size of the horizontal scrollbar's thumbnail in any unit.

### Value

Any floating-point value greater than zero and smaller than the difference between [XMAX](iup_xmax.md) and [XMIN](iup_xmin.md).

Default:: "0.1".

### Notes

LINEX, XMAX and XMIN are only updated in the scrollbar when DX is updated.

When the canvas is visible, a change in DX can generate a redraw in the horizontal scrollbar on the screen.
But it may generate a RESIZE_CB callback event if XAUTOHIDE=Yes.

A change in these values can affect the attribute [POSX](iup_posx.md).

### Affects

[IupCanvas](../elem/iup_canvas.md)

### See Also

[SCROLLBAR](iup_scrollbar.md)
