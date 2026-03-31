## IupMenuSeparator

Creates the separator interface element. It shows a line between two menu items.

### Creation

    Ihandle* IupMenuSeparator(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Notes

The separator is ignored when it is part of the definition of the items in a bar menu.

In GTK uses GtkSeparatorMenuItem, in GTK 4 uses GMenu separator section, in Windows uses InsertMenuItem, in WinUI uses XAML MenuFlyoutSeparator, in macOS uses NSMenuItem separatorItem, in Qt uses QAction separator, in EFL uses Elm_Menu separator, and in Motif uses xmSeparator.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupMenuItem](iup_menuitem.md), [IupSubMenu](iup_submenu.md), [IupMenu](iup_menu.md).
