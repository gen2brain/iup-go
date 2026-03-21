## SIZE (non-inheritable)

Specifies the element **User** size, and returns the **Current** size, in units proportional to the size of a character.

See the [Layout Guide](../layout.md) for more details on sizes.

### Value

"*width*x*height*", where width and height are integer values corresponding to the horizontal and vertical size, respectively, in characters fraction unit (see Notes below).

You can also set only one of the parameters by removing the other one and maintaining the separator "x", but this is the equivalent of setting the other value to 0.
For example, "x40" (height only = "0x40") or "40x" (width only = "40x0").

When this attribute is consulted, the **Current** size of the control is returned.
If both values are 0, then NULL is returned.

### Notes

The size units observe the following heuristics:

- Width in 1/4's of the average width of a character for the current **FONT** of each control.
- Height in 1/8's of the average height of a character for the current **FONT** of each control.

So, a SIZE="4x8" means 1 character width and 1 character height.

Notice that this is the **average** character size, the space occupied by a specific string is always different from its number of character times its average character size, except when using a monospaced font like Courier.
Usually for common strings this size is smaller than the actual size, so it is a good practice to leave more room than expected if you use the SIZE attribute.
For smaller font sizes, this difference is more noticeable than for larger font sizes.

When this attribute is changed, the [RASTERSIZE](iup_rastersize.md) attribute is automatically updated.

SIZE depends on [FONT](iup_font.md), so when **FONT** is changed and **SIZE** is set, then **RASTERSIZE** is also updated.

The average character size of the current **FONT** can be obtained from the [CHARSIZE](iup_font.md) attribute.

To obtain the last computed **Natural** size of the element in pixels, use the read-only attribute [NATURALSIZE](iup_naturalsize.md).

To obtain the **User** size of the element in pixels after it is mapped, use the attribute **USERSIZE**.

------------------------------------------------------------------------

A **User** size of "0x0" can be set, it can also be set using NULL.
If both values are 0 then NULL is returned.

If you wish to use the **User** size only as an initial size, change this attribute to NULL after the control is mapped, the returned size in **IupGetAttribute** will still be the **Current** size.

The element is NOT immediately repositioned. Call **IupRefresh** to update the dialog layout.

**IupMap** also updates the dialog layout even if it is already mapped, so calling it or calling **IupShow**, **IupShowXY** or **IupPopup** (they all call **IupMap**) will also update the dialog layout.

See the [Layout Guide](../layout.md) for mode details on sizes.

### Affects

All, except menus.

### See Also

[FONT](iup_font.md), [RASTERSIZE](iup_rastersize.md), [IupRefresh](../func/iup_refresh.md)
