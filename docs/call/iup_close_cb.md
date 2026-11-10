## CLOSE_CB

Called just before a dialog is closed when the user clicks the close button of the title bar or an equivalent action.
In Android, the Back button is the equivalent action; while the dialog's navigation drawer is open, Back closes the drawer instead.

### Callback

    int function(Ihandle *ih);

**ih**: identifier of the element that activated the event.

**Returns**: if IUP_IGNORE, it prevents the dialog from being closed.
If the dialog is destroyed in this callback, IUP_IGNORE must be returned. IUP_CLOSE will be processed.

### Affects

[IupDialog](../dlg/iup_dialog.md)
