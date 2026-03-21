## IupSeparator

Creates the separator interface element. It shows a line between two menu items.

### Creation

    Ihandle* IupSeparator(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Notes

The separator is ignored when it is part of the definition of the items in a bar menu.

In GTK uses GtkSeparatorMenuItem, in Windows uses InsertMenuItem, and in Motif uses xmSeparator.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupItem](iup_item.md), [IupSubMenu](iup_submenu.md), [IupMenu](iup_menu.md).
