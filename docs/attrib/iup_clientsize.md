## CLIENTSIZE (read-only) (non-inheritable)

Returns the client area size of a container.
It is the space available for positioning and sizing children, see the [Layout Guide](../layout.md).
It is the container **Current** size excluding the decorations (if any).

### Value

"*width*x*height*", where *width* and *height* are integer values corresponding to the horizontal and vertical size, respectively, in pixels.
If both values are 0 then "0x0" is returned.

### Affects

All elements that are containers except menus.

### Notes

(*) For **IupDialog** is NOT read-only, and it will re-define RASTERSIZE by adding the decorations to the given Client size.

For **IupHbox**, **IupVbox** and **IupGridBox** it consider the MARGIN attribute as a decoration.

For **IupSplit** returns the total area available for the two children.

### See Also

[SIZE](iup_size.md), [RASTERSIZE](iup_rastersize.md), [CLIENTOFFSET](iup_clientoffset.md)
