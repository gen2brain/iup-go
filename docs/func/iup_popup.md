## IupPopup

Shows a dialog or menu and restricts user interaction only to the specified element.
It is the equivalent of creating a Modal dialog is some toolkits.

If another dialog is shown after **IupPopup** using **IupShow**, then its interaction will not be inhibited.
Every **IupPopup** call creates a new popup level that inhibits all previous dialogs interactions, but does not disable new ones (even if they were disabled by the Popup, calling **IupShow** will re-enable the dialog because it will change its popup level).
IMPORTANT: The popup levels must be closed in the reverse order they were created or unpredictable results will occur.

For a dialog, this function will only return the control to the application after a callback returns IUP_CLOSE, **IupExitLoop** is called, or when the popup dialog is hidden, for example using **IupHide**.
For a menu it returns automatically after a menu item is selected.
IMPORTANT: If a menu item callback returns IUP_CLOSE, it will also end the current popup level dialog.

### Parameters/Return

    int IupPopup(Ihandle *ih, int x, int y);

**ih**: Identifier of a dialog or a menu.\
**x**: horizontal position of the top-left corner of the window or menu, relative to the origin of the main screen.
The following definitions can also be used:

- IUP_LEFT: Positions the element on the left border of the main screen
- IUP_CENTER: Centers the element on the main screen
- IUP_RIGHT: Positions the element on the right border of the main screen
- IUP_MOUSEPOS: Positions the element on the mouse cursor
- IUP_CENTERPARENT: Horizontally centralizes the dialog relative to its parent. Not valid for menus.
- IUP_CURRENT: use the current position of the dialog. Not valid for menus.
- IUP_LEFTPARENT: Positions the element on the left border of its parent. Not valid for menus.
- IUP_RIGHTPARENT: Positions the element on the right border of its parent. Not valid for menus.

**y**: vertical position of the top-left corner of the window or menu, relative to the origin of the main screen.
The following definitions can also be used:

- IUP_TOP: Positions the element on the top borderoffthe main screenthe main screen
- IUP_CENTER: Vertically centers the element on the main screen
- IUP_BOTTOM: Positions the element on the bottom border of the main screen
- IUP_MOUSEPOS: Positions the element on the mouse cursor
- IUP_CENTERPARENT: Vertically centralizes the dialog relative to its parent. Not valid for menus.
- IUP_CURRENT: use the current position of the dialog. Not valid for menus.
- IUP_TOPPARENT: Positions the element on the top border of its parent. Not valid for menus.
- IUP_BOTTOMPARENT: Positions the element on the bottom border of its parent. Not valid for menus.

**Returns:** IUP_NOERROR if successful.
Returns IUP_INVALID if not a dialog or menu. If there was error returns IUP_ERROR.

### Notes

It will call **IupMap** for the element.

The **x** and **y** values are the same as returned by the [SCREENPOSITION](../attrib/iup_screenposition.md) attribute.

 IUP_MOUSEPOS position is the same as returned by the [CURSORPOS](../attrib/iup_globals.md) global attribute.

See the [PLACEMENT](../dlg/iup_dialog.md) attribute for other position and show options.

When IUP_CENTERPARENT is used but PARENTDIALOG is not defined, then it is replaced by IUP_CENTER.

When IUP_CURRENT is used at the first time the dialog is shown, then it will be replaced by IUP_CENTERPARENT.

The main screen area does not include additional monitors.

**IupPopup** works just like **IupShowXY**, but it inhibits interaction with other dialogs that are visible and interrupts the processing until IUP_CLOSE is returned in a callback, **IupExitLoop** is called, or the popup dialog is hidden.
This is now a modal dialog. Although it interrupts the processing, it does not destroy the dialog when it ends.
To destroy the dialog, **IupDestroy** must be called.

The MODAL attribute of the dialog will be changed internally to return Yes.

In GTK and Motif the inactive dialogs will still be moveable, resizable and changeable their Z-order.
Although their contents will be inactive, the keyboard will be disabled, and they cannot be closed from the close box.

When called for an already visible dialog (modal or not) it will update its position and [PLACEMENT](../dlg/iup_dialog.md).
If the already visible dialog is not modal, then it will become modal and processing will be interrupted as a regular **IupPopup**.
If the already visible dialog is modal, then the function returns, and it will NOT interrupt processing, the dialog still will remain MODAL.
In other words, calling **IupPopup** a second time will just update the dialog position, and it will not interrupt processing, and calling **IupPopup** for a dialog shown with **IupShowXY** will turn it a modal dialog.

### See Also

[IupShowXY](iup_showxy.md), [IupShow](iup_show.md), [IupHide](iup_hide.md), [IupMap](iup_map.md), [IupDialog](../dlg/iup_dialog.md)
