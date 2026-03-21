## IupSpace

Creates a void element, which occupies an empty space.

It does not have a native representation.

### Creation

    Ihandle* IupSpace(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**WID** (read-only): returns -1 if mapped.

> 
>
> ------------------------------------------------------------------------

[SIZE](../attrib/iup_size.md), [RASTERSIZE](../attrib/iup_rastersize.md), [EXPAND](../attrib/iup_expand.md), [FONT](../attrib/iup_font.md), [POSITION](../attrib/iup_position.md), [MINSIZE](../attrib/iup_minsize.md), [MAXSIZE](../attrib/iup_maxsize.md), [THEME](../attrib/iup_theme.md): also accepted.

### Notes

When an **IupFill** is inside a **IupVbox** or **IupHbox**, it will affect the expansion of the box because it is always expandable.
Even when you set its size to a given value, it will still affect the layout, because it is always marked as an expandable element.

**IupSpace** will simply occupy a space in the layout. It does not have a natural size, it is 0x0 by default.
It can be expandable or not, EXPAND will work as a regular element.
The attributes SIZE and RASTERSIZE can be normally set.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupHbox](iup_hbox.md), [IupVbox](iup_vbox.md).
