## CLIENTOFFSET (read-only) (non-inheritable)

Returns the native container internal offset to the **Client** area, see the [Layout Guide](../layout.md).
Useful for **IupFrame**, **IupTabs** and **IupDialog** that have decorations.
It can also be consulted in other containers, it will simply return "0x0".

This attribute can be used in conjunction with the POSITION attribute of a child so the coordinates of a child relative to the native parent top-left corner can be obtained.

### Value

"*dx*x*dy*", where *dx* and *dy* are integer values corresponding to the horizontal and vertical offsets, respectively, in pixels.

### Affects

All elements that are containers, except menus.

### Notes

In GTK and Motif, for the **IupDialog**, the dy value is negative when there is a menu.
This occurs because in those systems the menu is placed inside the Client Area and all children must be placed below the menu.
In Windows, it will return 0x0, except when CUSTOMFRAMEDRAW is used.

In Windows, for the **IupFrame**, the value is always "0x0" the position of the child is still relative to the top-left corner of the frame.
This is automatically compensated in calculation of the POSITION attribute.

### See Also

[SIZE](iup_size.md), [RASTERSIZE](iup_rastersize.md), [CLIENTSIZE](iup_clientsize.md), [POSITION](iup_position.md)
