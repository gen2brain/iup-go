## VISIBLE

Shows or hides the element.

### Value

"YES" (visible), "NO" (hidden).

Default: "YES"

### Notes

An interface element is only visible if its native parent is also visible.

For an [IupDialog](../dlg/iup_dialog.md) the default is "NO"; it is shown with [IupShow](../func/iup_show.md), [IupShowXY](../func/iup_showxy.md) or [IupPopup](../func/iup_popup.md).

### Affects

All controls that have visual representation, except menus.
