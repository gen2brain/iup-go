## IupFill

Creates a void element, which dynamically occupies empty spaces always trying to expand itself.
Its parent should be an **IupHbox,** an **IupVbox** or a **IupGridBox**, or else this type of expansion will not work.
If an EXPAND is set on at least one of the other children of the box, then the fill expansion is ignored.

It does not have a native representation.

### Creation

    Ihandle* IupFill(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

[EXPAND](../attrib/iup_expand.md) (non-inheritable)(read-only): If **User** size is not defined, then when inside a **IupHbox/IupGridBox** EXPAND is HORIZONTAL, when inside a **IupVbox** EXPAND is VERTICAL.
If **User** size is defined, then EXPAND is NO.

[SIZE](../attrib/iup_size.md) / [RASTERSIZE](../attrib/iup_rastersize.md) (non-inheritable): Defines the width, if inside a **IupHbox**, or the height, if it is inside a **IupVbox**.
The standard format "wxh" can also be used, but width will be ignored if inside a **IupVbox** and height will be ignored if inside a **IupHbox**.
When consulted behaves as the standard SIZE/RASTERSIZE attributes.

**WID** (read-only): returns -1 if mapped.

> 
>
> ------------------------------------------------------------------------

[FONT](../attrib/iup_font.md), [POSITION](../attrib/iup_position.md), [MINSIZE](../attrib/iup_minsize.md), [MAXSIZE](../attrib/iup_maxsize.md), [THEME](../attrib/iup_theme.md): also accepted.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupHbox](iup_hbox.md), [IupVbox](iup_vbox.md).
