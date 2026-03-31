## IupMessageDlg

Creates the Message Dialog element. It is a predefined dialog for displaying a message.
The dialog can be shown with the IupPopup function only.

### Creation

    Ihandle* IupMessageDlg(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**BUTTONDEFAULT**: Number of the default button.
Can be "1", "2" or "3". "2" is valid only for "RETRYCANCEL", "OKCANCEL" and "YESNO" button configurations. "3" is valid only for "YESNOCANCEL".
Default: "1".

**BUTTONRESPONSE**: Number of the pressed button. Can be "1", "2" or "3". Default: "1".

**BUTTONS**: Buttons configuration. Can have values: "OK", "OKCANCEL", "RETRYCANCEL", "YESNO", or "YESNOCANCEL".
Default: "OK". Additionally, the "Help" button is displayed if the HELP_CB callback is defined.

**DIALOGTYPE**: Type of dialog defines which icon will be displayed beside the message text.
Can have values: "MESSAGE" (No Icon), "ERROR" (Stop-sign), "WARNING" (Exclamation-point), "QUESTION" (Question-mark) or "INFORMATION" (Letter "i").
Default: "MESSAGE".

[PARENTDIALOG](../attrib/iup_parentdialog.md) (creation-only): Name of a dialog to be used as parent.
This dialog will always be in front of the parent dialog.
If not defined in Motif the dialog could not be modal.

[TITLE](../attrib/iup_title.md): Dialog title.

**VALUE**: Message text.

### Callbacks

[HELP_CB](../call/iup_help_cb.md): Action generated when the Help button is pressed.

### Notes

The **IupMessageDlg** is a native pre-defined dialog not altered by **IupSetLanguage**.

To show the dialog, use function **IupPopup**.

The dialog is mapped only inside **IupPopup**, **IupMap** does nothing.

In Windows, the position (x,y) used in **IupPopup** is ignored and the dialog is always centered on the screen.

The **IupMessage** function simply creates and popups a **IupMessageDlg**.

In Windows, each different dialog type is always associated with a different beep sound.

In Windows, if PARENTDIALOG is specified, then it will be modal relative only to its parent.

In GTK uses the gtk_message_dialog, in Windows uses MessageBox, in FLTK uses fl_message/fl_choice, and in Motif uses xmMessageBox.

### Examples

    Ihandle* dlg = IupMessageDlg();

    IupSetAttribute(dlg, "DIALOGTYPE", "WARNING");
    IupSetAttribute(dlg, "TITLE", "IupMessageDlg Test");
    IupSetAttribute(dlg, "BUTTONS", "OKCANCEL");
    IupSetAttribute(dlg, "VALUE", "Message Text\nSecond Line");
    IupSetCallback(dlg, "HELP_CB", (Icallback)help_cb);

    IupPopup(dlg, IUP_CURRENT, IUP_CURRENT);

    printf("BUTTONRESPONSE(%s)\n", IupGetAttribute(dlg, "BUTTONRESPONSE"));

    IupDestroy(dlg);  

**Windows XP**
![](images/messagedlg_win.png)

**Motif/Mwm**
![](images/messagedlg_mot.png)

**GTK/GNOME**
![](images/messagedlg_gtk.png)

### See Also

[IupMessage](iup_message.md), [IupListDialog](iup_listdialog.md), [IupAlarm](iup_alarm.md), [IupGetFile](iup_getfile.md), [IupPopup](../func/iup_popup.md)
