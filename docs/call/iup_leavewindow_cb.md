## LEAVEWINDOW_CB

Action generated when the mouse leaves the native element.

### Callback

    int function(Ihandle *ih);

**ih**: identifier of the element that activated the event.

### Notes

When the cursor is moved from one element to another, the call order in all platforms will be first the LEAVEWINDOW_CB callback of the old control followed by the ENTERWINDOW_CB callback of the new control.

If the mouse button is hold pressed and the cursor moves outside the element, the behavior is system-dependent.
In Windows, the LEAVEWINDOW_CB/ENTERWINDOW_CB callbacks are NOT called, in GTK the callbacks are called.

### Affects

All controls with user interaction.

### See Also

[ENTERWINDOW_CB](iup_enterwindow_cb.md)
