## CLOSE_CB

Called just before a dialog is closed when the user clicks the close button of the title bar or an equivalent action.

### Callback

    int function(Ihandle *ih);

**ih**: identifies the element that activated the event.

**Returns**: if IUP_IGNORE, it prevents the dialog from being closed.
If you destroy the dialog in this callback, you must return IUP_IGNORE. IUP_CLOSE will be processed.

### Affects

[IupDialog](../dlg/iup_dialog.md)
