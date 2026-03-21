## IupMessage

Shows a modal dialog containing a message. It simply creates and popups a **IupMessageDlg**.

### Creation and Show

    void IupMessage(const char *title, const char *message);
    void IupMessagef(const char *title, const char *format, ...);
    void IupMessageV(const char *title, const char *format, va_list arglist);

**title**: dialog title\
**message**: text message contents\
**format**: same format as the C sprintf function

### Notes

The **IupMessage** function shows a dialog centralized on the screen, showing the message and the “OK” button.
The ‘\n’ character can be added to the message to indicate line change.

The dialog uses a global attribute called "PARENTDIALOG" as the parent dialog if it is defined.
It also uses a global attribute called "ICON" as the dialog icon if it is defined (used only in Motif, in Windows MessageBox does not have an icon in the title bar).

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupGetFile](iup_getfile.md), [IupListDialog](iup_listdialog.md), [IupAlarm](iup_alarm.md), [IupMessageDlg](iup_messagedlg.md)
