## CLIENTOFFSET (read-only) (non-inheritable)

Returns the native container internal offset to the **Client** area, see the [Layout Guide](../layout.md#layout-guide).
Useful for **IupFrame**, **IupTabs** and **IupDialog** that have decorations.
It can also be consulted in other containers, it will simply return "0x0".

This attribute can be used in conjunction with the POSITION attribute of a child so the coordinates of a child relative to the native parent top-left corner can be obtained.

### Value

"*dx*x*dy*", where *dx* and *dy* are integer values corresponding to the horizontal and vertical offsets, respectively, in pixels.

### Affects

All elements that are containers, except menus.

### Notes

For the **IupDialog**: in GTK 3, Motif and EFL, dy is minus the menu height when there is a menu.
In GTK 4 and WebAssembly, dx is the border and dy is border+caption+menu.
In Win32 and macOS the same values are returned when CUSTOMFRAMEDRAW=YES, "0x0" otherwise.
In the other drivers it is "0x0".

For the **IupFrame**: in Win32, WinUI, EFL, iOS and WebAssembly the value is "0x0" and the POSITION of a child is already relative to the top-left corner of the frame.
In the other drivers it is the frame border plus the title height.

### See Also

[SIZE](iup_size.md), [RASTERSIZE](iup_rastersize.md), [CLIENTSIZE](iup_clientsize.md), [POSITION](iup_position.md)
