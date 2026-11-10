## IupShareDlg

Creates the Share Dialog element. It is a predefined dialog that shares text, a URL or a file through the system share sheet.
The dialog can be shown with the **IupPopup** function only.

### Creation

    Ihandle* IupShareDlg(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**FILE**: path of a file to share.

**STATUS** (read-only): "1" if the user picked a target, "0" if the user closed the share sheet, "-1" if sharing failed or is not supported.

**TEXT**: text to share.

[TITLE](../attrib/iup_title.md): title shown in the share sheet.
Supported in Win32 and WinUI.

**URL**: URL to share.

### Notes

**IupPopup** returns after the user picks a target or closes the share sheet.

The dialog is mapped only inside **IupPopup**, **IupMap** does nothing.

In Win32 and WinUI, it uses DataTransferManager, in macOS NSSharingServicePicker, in Android the ACTION_SEND chooser, and in iOS UIActivityViewController.

Supported in Win32, WinUI, macOS, Android and iOS.

### Examples

    Ihandle* dlg = IupShareDlg();
    IupSetAttribute(dlg, "URL", "https://github.com/gen2brain/iup-go");

    IupPopup(dlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

    if (IupGetInt(dlg, "STATUS") == 1)
      printf("SHARED\n");

    IupDestroy(dlg);

[Browse for Example Files](../../examples/)

### See Also

[IupFileDlg](iup_filedlg.md), [IupPopup](../func/iup_popup.md)
