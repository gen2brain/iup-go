## DY

Size of the vertical scrollbar's thumbnail in any unit.

### Value

Any floating-point value greater than zero and smaller than the difference between [YMAX](iup_ymax.md) and [YMIN](iup_ymin.md).

Default:: "0.1".

### Notes

LINEY, YMAX and YMIN are only updated in the scrollbar when DY is updated.

When the canvas is visible, a change in DY can generate a redraw in the horizontal scrollbar on the screen.
But it may generate a RESIZE_CB callback event if YAUTOHIDE=Yes.

A change in these values can affect the attribute [POSY](iup_posy.md).

### Affects

[IupCanvas](../elem/iup_canvas.md)

### See Also

[SCROLLBAR](iup_scrollbar.md)
