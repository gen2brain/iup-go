## IupGetFile

Shows a modal dialog of the native interface system to select a filename.
Uses the **IupFileDlg** element.

### Creation and Show

    int IupGetFile(char *filename);

**filename**: This parameter is used as an input value to define the default filter and directory.
Example: "../docs/*.txt". As an output value, it is used to contain the filename entered by the user.

**Returns:** a **status** code, whose values can be:

> "1": New file.\
> "0": Normal, existing file.\
> "-1": Operation cancelled.

### Notes

The function does not allocate memory space to store the complete filename entered by the user.
Therefore, the filename parameter must be large enough to contain the directory and file names.
The string is limited to 4096 characters.

The function will reuse the directory from one call to another, so in the next call will open in the directory of the last selected file.

The dialog uses a global attribute called "PARENTDIALOG" as the parent dialog if it is defined.
It also uses a global attribute called "ICON" as the dialog icon if it is defined.

For a more controlled dialog, use directly the [IupFileDlg](iup_filedlg.md) element.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupFileDlg](iup_filedlg.md), [IupMessage](iup_message.md), [IupListDialog](iup_listdialog.md), [IupAlarm](iup_alarm.md), [IupSetLanguage](../func/iup_setlanguage.md).
