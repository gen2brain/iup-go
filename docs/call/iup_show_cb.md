## SHOW_CB

Called right after the dialog is shown, hidden, maximized, minimized or restored from minimized/maximized.
This callback is called when those actions were performed by the user or programmatically by the application.

### Callback

    int function(Ihandle *ih, int state);

**ih**: identifier of the element that activated the event.\
**state:** indicates which of the following situations generated the event:

> IUP_HIDE\
> IUP_SHOW\
> IUP_RESTORE (was minimized or maximized)\
> IUP_MINIMIZE\
> IUP_MAXIMIZE (not received in Motif when activated from the maximize button)

**Returns**: IUP_CLOSE will be processed.

### Affects

[IupDialog](../dlg/iup_dialog.md)
