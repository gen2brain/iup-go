## IupCbox

Creates a void container for position elements in absolute coordinates.
It is a **concrete** layout container.

It does not have a native representation.

The **IupCbox** is equivalent of a **IupVbox** or **IupHbox** where all the children have the [FLOATING](../attrib/iup_floating.md) attribute set to YES, but children must use CX and CY attributes instead of the POSITION attribute.

### Creation

    Ihandle* IupCbox(Ihandle* child, ...);
    Ihandle* IupCboxV(Ihandle* child, va_list arglist);
    Ihandle* IupCboxv(Ihandle** children);

**child**, ... : List of the identifiers that will be placed in the box.
NULL must be used to define the end of the list in C.
It can be empty, but in C must have at least the NULL terminator.

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

[EXPAND](../attrib/iup_expand.md) (non-inheritable): The default value is "YES".

[SIZE](../attrib/iup_size.md) / [RASTERSIZE](../attrib/iup_rastersize.md) (non-inheritable): Must be defined for each child.
If not defined for the box, then it will be the bounding box that includes all children in their position.

**WID** (read-only): returns -1 if mapped.

> 
>
> ------------------------------------------------------------------------

[FONT](../attrib/iup_font.md), [CLIENTSIZE](../attrib/iup_clientsize.md), [CLIENTOFFSET](../attrib/iup_clientoffset.md), [POSITION](../attrib/iup_position.md), [MINSIZE](../attrib/iup_minsize.md), [MAXSIZE](../attrib/iup_maxsize.md), [THEME](../attrib/iup_theme.md): also accepted.

### Attributes (at Children)

**CX, CY** (non-inheritable) **(at children only)**: Position in pixels of the child relative to the top-left corner of the box.
Must be set for each child inside the box.

### Notes

The box can be created with no elements and be dynamic filled using [IupAppend](../func/iup_append.md) or [IupInsert](../func/iup_insert.md).

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupVbox](iup_vbox.md), [IupHbox](iup_hbox.md)
