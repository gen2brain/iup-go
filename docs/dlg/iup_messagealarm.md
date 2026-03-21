## IupMessageAlarm

Shows a modal dialog containing a question message, similar to **IupAlarm**.
It simply creates and popup a **IupMessageDlg** with DIALOGTYPE=QUESTION.

### Creation and Show

    void IupMessageAlarm(Ihandle* parent, const char *title, const char *message, const char* buttons);

**parent**: parent dialog, can be NULL.\
**title**: dialog’s title, can be NULL.  It can be a language pre-defined string without the "_@" prefix.**\
message**: text message contents.  It can be a language pre-defined string without the "_@" prefix.\
**buttons**: list of buttons. Can have values: "OK", "OKCANCEL", "RETRYCANCEL", "YESNO", or "YESNOCANCEL".

**Returns:** the number of the **button** selected by the user (1, 2 or 3).

### Notes

If parent is NULL, the title defaults to "Attention!" and tries the global attribute "PARENTDIALOG" as the parent dialog.

The dialog is shown centered relative to its parent.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupGetFile](iup_getfile.md), [IupListDialog](iup_listdialog.md), [IupAlarm](iup_alarm.md), [IupMessage](iup_message.md), [IupMessageDlg](iup_messagedlg.md)
