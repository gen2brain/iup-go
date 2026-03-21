## IupGetColor

Shows a modal dialog which allows the user to select a color.
Based on [IupColorDlg](iup_colordlg.md).

### Creation and Show

    int IupGetColor(int x, int y, unsigned char *r, unsigned char *g, unsigned char *b);

x, y: x, y values of the **IupPopup** function.\
**r, g, b**: Pointers to variables that will receive the color selected by the user if the OK button is pressed.
The value in the variables at the moment the function is called defines the color being selected when the dialog is shown.
If the OK button is not pressed, the r, g and b values are not changed. These values cannot be NULL.

**Returns:** in C a code 1 if the OK button is pressed, or 0 otherwise.

### Notes

The dialog uses a global attribute called "PARENTDIALOG" as the parent dialog if it is defined.
It also uses a global attribute called "ICON" as the dialog icon if it is defined.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupMessage](iup_message.md), [IupListDialog](iup_listdialog.md), [IupAlarm](iup_alarm.md), [IupGetFile](iup_getfile.md).
