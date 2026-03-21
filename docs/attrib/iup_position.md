## POSITION (non-inheritable)

The position of the element relative to the origin of the **Client** area of the native parent.
If you add the CLIENTOFFSET attribute of the native parent, you can obtain the coordinates relative to the **Window** area of the native parent.
See the [Layout Guide](../layout.md).

It will be changed during the layout computation, except when FLOATING=YES or when used inside a concrete layout container.

### Value

"x,y", where *x* and *y* are integer values corresponding to the horizontal and vertical position, respectively, in pixels.

### Affects

All, except menus.

### See Also

[SIZE](iup_size.md), [RASTERSIZE](iup_rastersize.md), [FLOATING](iup_floating.md), [CLIENTOFFSET](iup_clientoffset.md)
