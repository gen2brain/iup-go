## KEY

Associates a key to a menu item or submenu.
Such key works as a shortcut when the menu is open, this is not a hot key.

### Value

String containing a key description. It is a string representation of an IUP key code.
Please refer to the [Keyboard Codes](iup_keyboard_codes.md) table for a list of the possible values.

Default: NULL

### Notes

IUP automatically underlines the first appearance of the chosen menu letter.
For such, the chosen letter must necessarily be a part of the menu text.

In Windows, when used will also set an underscore on the respective letter of the submenu title.

The key will be used when navigating in the parent menu that contains the item.
If the same character key is present in the title, then it will be underlined.

In the menu bar, some systems automatically associate the ALT+<letter> combination for the chosen letter.

Be careful not to misuse this attribute in relation to [K_ANY](../call/iup_k_any.md) or K_* callbacks.

### Affects

[IupItem](../elem/iup_item.md), [IupSubMenu](../elem/iup_submenu.md).
