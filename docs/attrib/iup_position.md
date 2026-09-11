## POSITION (non-inheritable)

The position of the element relative to the origin of the **Client** area of the native parent.
If you add the CLIENTOFFSET attribute of the native parent, you obtain the coordinates relative to the **Window** area of the native parent.
See the [Layout Guide](../layout.md#layout-guide).

It is computed during the layout, so a value set on the element is replaced when the layout is updated.
The exception is FLOATING=YES: the layout does not position a floating element, so it keeps the value you set.

### Value

"x,y", where *x* and *y* are integer values corresponding to the horizontal and vertical position, respectively, in pixels.

### Affects

All elements, except menus and dialogs.

### See Also

[SIZE](iup_size.md), [RASTERSIZE](iup_rastersize.md), [FLOATING](iup_floating.md), [CLIENTOFFSET](iup_clientoffset.md)
