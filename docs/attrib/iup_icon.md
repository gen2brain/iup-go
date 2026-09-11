## ICON

Dialog's icon. This icon will be used when the dialog is minimized among other places by the native system.

### Value

Name of a IUP image.

Default: NULL

### Notes

Icon sizes are usually less than or equal to 32x32.
In Windows, an icon with 16x16, 32x32 and 48x48 images of 32 bpp covers every place the system shows it.

In GTK 4 the value is the name of an icon in the icon theme, not an IUP image.

In macOS and Haiku the icon is the application icon, shown in the Dock and in the Deskbar; setting it on any dialog replaces it for the whole application.

In Android and iOS the icon is shown in the title bar next to the title.

On Wayland the taskbar and window switcher icon comes from the `.desktop` file matched by the [APPID](iup_globals.md#appid) global, not from this attribute.

Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate an image to a name.

### Affects

[IupDialog](../dlg/iup_dialog.md)

### See Also

[IupImage](../elem/iup_image.md)
