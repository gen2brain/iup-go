## KEY

Underlines a character of the TITLE of a menu item or submenu.
Pressing that character while the parent menu is open activates the item.
For a submenu in the menu bar, Alt+character opens it from any control in the dialog.
Same as writing "&" before that character in TITLE, see [IupMenuItem](../elem/iup_menuitem.md).
Deprecated, use the "&" in TITLE.

Not supported in Cocoa, EFL, Android and iOS.
In Haiku the underline and the key are active when the system menu preference shows triggers, and Alt+character does not open a menu bar submenu.

### Value

A single character. The first occurrence of it in TITLE is underlined.
When the character is not in TITLE nothing is underlined.

Default: NULL

### Notes

It is applied when TITLE is set. After mapping, a new KEY takes effect when TITLE is set again.

### Affects

[IupMenuItem](../elem/iup_menuitem.md), [IupSubMenu](../elem/iup_submenu.md).
