## IupRadio

Creates a void container for grouping mutual exclusive toggles.
Only one of its descendent toggles will be active at a time. The toggles can be at any composition.

It does not have a native representation.

### Creation

    Ihandle* IupRadio(Ihandle *child);

**child**: Identifier of an interface element.
Usually it is a vbox or a hbox containing the toggles associated to the radio. It can be NULL.

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

[EXPAND](../attrib/iup_expand.md) (non-inheritable): The default value is "YES".

**VALUE** (non-inheritable): name identifier of the active toggle.
The name is set by means of [IupSetHandle](../func/iup_sethandle.md).
When consulted if the toggles are not mapped into the native system the return value may be NULL or invalid.

**VALUE_HANDLE** (non-inheritable): Changes the active toggle.
The value passed must be the handle of a child contained in the radio.
When consulted if the toggles are not mapped into the native system the return value may be NULL or invalid.

**WID** (read-only): returns -1 if mapped.

> 
>
> ------------------------------------------------------------------------

[FONT](../attrib/iup_font.md), [CLIENTSIZE](../attrib/iup_clientsize.md), [CLIENTOFFSET](../attrib/iup_clientoffset.md), [POSITION](../attrib/iup_position.md), [MINSIZE](../attrib/iup_minsize.md), [MAXSIZE](../attrib/iup_maxsize.md), [VISIBLE](../attrib/iup_visible.md), [THEME](../attrib/iup_theme.md): also accepted.

### Notes

The radio can be created with no elements and be dynamic filled using [IupAppend](../func/iup_append.md) or [IupInsert](../func/iup_insert.md). 

A toggle that is a child of an **IupRadio** automatically receives a name when it is mapped into the native system.

Currently, **IupFlatButton** with TOGGLE=YES, **IupToggle**, and **IupGLToggle** are affected when inside a **IupRadio**.

The **IGNORERADIO** can be used in any of these children types to disable this functionally.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupToggle](iup_toggle.md)
