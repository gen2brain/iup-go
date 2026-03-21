## MAXSIZE (non-inheritable)

Specifies the element maximum size in pixels during the layout process.

See the [Layout Guide](../layout.md) for more details on sizes.

### Value

"*width*x*height*", where *width* and *height* are integer values corresponding to the horizontal and vertical size, respectively, in pixels.

You can also set only one of the parameters by removing the other one and maintaining the separator "x", but this is the equivalent of setting the other value to 65535.
For example, "x40" (height only = "65535x40") or "40x" (width only = "40x65535").

Default: 65535x65535

### Affects

All, except menus.

### Notes

The limits are applied during the layout computation.
It will limit the Natural size and the Current size.

If the element can be expanded, then its empty space will NOT be occupied by other controls, although its size will be limited.

In the **IupDialog** will also limit the interactive resize of the dialog.

See the [Layout Guide](../layout.md) for mode details on sizes.

### See Also

[RASTERSIZE](iup_rastersize.md), [MINSIZE](iup_minsize.md)
