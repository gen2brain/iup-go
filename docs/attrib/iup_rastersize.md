## RASTERSIZE (non-inheritable)

Specifies the element **User** size, and returns the **Current** size, in pixels.

See the [Layout Guide](../layout.md) for more details on sizes.

### Value

"*width*x*height*", where *width* and *height* are integer values corresponding to the horizontal and vertical size, respectively, in pixels.

You can also set only one of the parameters by removing the other one and maintaining the separator "x", but this is the equivalent of setting the other value to 0.
For example, "x40" (height only = "0x40") or "40x" (width only = "40x0").

When this attribute is consulted, the **Current** size of the control is returned.
If both values are 0, then NULL is returned.

### Affects

All, except menus.

### Notes

When this attribute is set, it resets the [SIZE](iup_size.md) attribute.
So changes to the [FONT](iup_font.md) attribute will not affect the **User** size of the element.

To obtain the last computed **Natural** size of the control in pixels, use the read-only attribute [NATURALSIZE](iup_naturalsize.md).

To obtain the **User** size of the element in pixels after it is mapped, use the attribute **USERSIZE**.

------------------------------------------------------------------------

A **User** size of "0x0" can be set, it can also be set using NULL.
If both values are 0 then NULL is returned.

If you wish to use the **User** size only as an initial size, change this attribute to NULL after the control is mapped, the returned size in **IupGetAttribute** will still be the **Current** size.

The element is NOT immediately repositioned. Call **IupRefresh** to update the dialog layout.

**IupMap** also updates the dialog layout even if it is already mapped, so calling it or calling **IupShow**, **IupShowXY** or **IupPopup** (they all call **IupMap**) will also update the dialog layout.

See the [Layout Guide](../layout.md) for mode details on sizes.

### See Also

[SIZE](iup_size.md), [FONT](iup_font.md)
