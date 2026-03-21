## IupItem

Creates an item of the menu interface element. When selected, it generates an action.

### Creation

    Ihandle* IupItem(const char *title, const char *action);

**title**: Text to be shown on the item. It can be NULL. It will set the TITLE attribute.\
**action**: Name of the action generated when the item is selected. It can be NULL.

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**AUTOTOGGLE** (non-inheritable): enables the automatic toggle of VALUE state when the item is activated.
Default: NO.

[KEY](../attrib/iup_key.md) (non-inheritable): Underlines a key character in the submenu title.
It is updated only when TITLE is updated.  Deprecated**, use the mnemonic support directly in the TITLE attribute.
**HIDEMARK**: If enabled the item cannot be checked, since the checkbox will not be shown.
If all items in a menu enable it, then no empty space will be shown in front of the items.
If your item will not be marked you must set HIDEMARK=YES, but if VALUE is defined the default goes back to NO.
Default: NO.

**IMAGE** (non-inheritable): Image name of the check mark image when VALUE=OFF.
In Windows, an item in a menu bar cannot have a check mark. Ignored if item in a menu bar.
A recommended size would be 16x16 to fit the image in the menu item.
In Windows, if larger than the check mark area it will be cropped.

**IMPRESS** (non-inheritable): Image name of the check mark image when VALUE=ON.

[TITLE](../attrib/iup_title.md) (non-inheritable): Item text.
The "&" character can be used to define a mnemonic, the next character will be used as key.
Use "&&" to show the "&" character instead on defining a mnemonic.
When in a menu bar an item that has a mnemonic can be activated from any control in the dialog using the "Alt+key" combination.

The text also accepts the control character '\t' to force text alignment to the right after this character.
This is used to add shortcut keys to the menu, aligned to the right, ex: "Save\tCtrl+S", but notice that the shortcut key (also known as Accelerator or Hot Key) still has to be implemented.
To implement a shortcut, use the K_* callbacks in the dialog.

**TITLEIMAGE** (non-inheritable): Image name of the title image.
In Windows, it appears before of the title text and after the check mark area (so both title and title image can be visible).
In Motif, it replaces the text, and only images will be possible to set (TITLE will be hidden).
In GTK, it will appear on the check mark area.

**VALUE** (non-inheritable): Indicates the item's state.
When the value is ON, a mark will be displayed to the left of the item. Default: OFF.
An item in a menu bar cannot have a check mark. When IMAGE is used, the checkmark is not shown.
See the item AUTOTOGGLE attribute and the menu [RADIO](iup_menu.md) attribute.

[WID](../attrib/iup_wid.md) (non-inheritable): In Windows, returns the HMENU of the parent menu.

> 
>
> ------------------------------------------------------------------------

[ACTIVE](../attrib/iup_active.md), [THEME](../attrib/iup_theme.md): also accepted.

### Callbacks

[ACTION](../call/iup_action.md): Action generated when the item is selected. IUP_CLOSE will be processed.
Even if inside a popup menu when IUP_CLOSE is returned, the current popup dialog or the main loop will be closed.

[HIGHLIGHT_CB](../call/iup_highlight_cb.md): Action generated when the item is highlighted.

------------------------------------------------------------------------

[MAP_CB](../call/iup_map_cb.md), [UNMAP_CB](../call/iup_unmap_cb.md), [DESTROY_CB](../call/iup_destroy_cb.md), [HELP_CB](../call/iup_help_cb.md): common callbacks are supported.

### Notes

Menu items are activated using the Enter key.

In Motif and GTK, the text font will be affected by the dialog font when the menu is mapped.

To have a menu item that can be marked, you must set the VALUE attribute to ON or OFF, or set HIDEMARK=NO, before mapping the control.

In GTK uses GtkMenuItem/GtkCheckMenuItem, in GTK 4 uses GMenu/GSimpleAction, in Windows uses InsertMenuItem, in WinUI uses XAML MenuFlyoutItem, in macOS uses NSMenuItem, in Qt uses QAction, in EFL uses Elm_Menu_Item, and in Motif uses xmCascadeButton/xmToggleButton.

### Examples

[Browse for Example Files](../../examples/)

See the **IupMenu** element for screenshots.

### See Also

[IupSeparator](iup_separator.md), [IupSubmenu](iup_submenu.md), [IupMenu](iup_menu.md).
