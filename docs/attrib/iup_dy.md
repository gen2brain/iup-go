## DY

Size of the thumb in the vertical scrollbar, in any unit.

### Value

Any floating-point value greater than zero and smaller than the difference between [YMAX](iup_ymax.md) and [YMIN](iup_ymin.md).

Default: "0.1".

### Notes

LINEY, YMAX and YMIN are only updated in the scrollbar when DY is updated.

When the canvas is visible, a change in DY can generate a redraw in the vertical scrollbar on the screen.
It may also generate a RESIZE_CB callback event if YAUTOHIDE=YES.

A change in these values can affect the attribute [POSY](iup_posy.md).

### Affects

[IupCanvas](../elem/iup_canvas.md)

### See Also

[SCROLLBAR](iup_scrollbar.md)
