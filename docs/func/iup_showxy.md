## IupShowXY

Displays a dialog in a given position on the screen.

### Parameters/Return

    int IupShowXY(Ihandle *ih, int x, int y);

**ih**: identifier of the dialog.\
**x**: horizontal position of the top-left corner of the window, relative to the origin of the main screen.
The following definitions can also be used:

- IUP_LEFT: Positions the dialog on the left border of the main screen
- IUP_CENTER: Horizontally centralizes the dialog on the main screen
- IUP_RIGHT: Positions the dialog on the right border of the main screen
- IUP_MOUSEPOS: Positions the dialog on the mouse position
- IUP_CENTERPARENT: Horizontally centralizes the dialog relative to its parent
- IUP_CURRENT: use the current position of the dialog.
- IUP_LEFTPARENT: Positions the element on the left border of its parent. Not valid for menus.
- IUP_RIGHTPARENT: Positions the element on the right border of its parent. Not valid for menus.

**y**: vertical position of the top-left corner of the window, relative to the origin of the main screen.
The following definitions can also be used:

- IUP_TOP: Positions the dialog on the top border of the main screen
- IUP_CENTER: Vertically centralizes the dialog on the main screen
- IUP_BOTTOM: Positions the dialog on the bottom border of the main screen
- IUP_MOUSEPOS: Positions the dialog on the mouse position
- IUP_CENTERPARENT: Vertically centralizes the dialog relative to its parent
- IUP_CURRENT: use the current position of the dialog.
- IUP_TOPPARENT: Positions the element on the top border of its parent. Not valid for menus.
- IUP_BOTTOMPARENT: Positions the element on the bottom border of its parent. Not valid for menus.

**Returns:** IUP_NOERROR if successful.
Returns IUP_INVALID if not a dialog.  If there was an error returns IUP_ERROR.

### Notes

Will call **IupMap** for the element.

**x** and **y** positions are the same as returned by the [SCREENPOSITION](../attrib/iup_screenposition.md) attribute.

 IUP_MOUSEPOS position is the same as returned by the [CURSORPOS](../attrib/iup_globals.md) global attribute.

See the [PLACEMENT](../dlg/iup_dialog.md) attribute for other position and show options.

When IUP_CENTERPARENT is used but PARENTDIALOG is not defined, then it is replaced by IUP_CENTER.

When IUP_CURRENT is used at the first time the dialog is shown, then it will be replaced by IUP_CENTERPARENT.

The main screen area does not include additional monitors.

When called for an already visible dialog (MODAL or not) it will update its position and [PLACEMENT](../dlg/iup_dialog.md).
If the already visible dialog is not modal, but it was disabled by one (a call to **IupPopup** for another dialog, after this one was shown), then it will be now enabled for interaction.

### See Also

[IupShow](iup_show.md), [IupHide](iup_hide.md), [IupPopup](iup_popup.md), [IupMap](iup_map.md), [IupDialog](../dlg/iup_dialog.md)
