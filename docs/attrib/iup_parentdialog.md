## PARENTDIALOG

The parent dialog of a dialog.

### Value

Name of a dialog to be used as parent.

Default: NULL.

### Notes

This dialog will always be in front of the parent dialog.
If the parent is minimized, this dialog is automatically minimized.
The parent dialog must be mapped before mapping the child dialog.

If PARENTDIALOG is not defined, then the NATIVEPARENT attribute is consulted.
This one must be a native handle of an existing dialog.

It can be changed after the dialog is mapped, but only if already set before map.

Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate a dialog to a name.

**IMPORTANT**: When the parent is destroyed, the child dialog is also destroyed, BUT the CLOSE_CB callback of the child dialog is NOT called.
The application must take care of destroying the child dialogs before destroying the parent.
This is usually done when CLOSE_CB of the parent dialog is called.

### Affects

[IupDialog](../dlg/iup_dialog.md)
