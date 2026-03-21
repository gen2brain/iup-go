## ICON

Dialog's icon. This icon will be used when the dialog is minimized among other places by the native system.

### Value

Name of a IUP image.

Default: NULL

### Notes

Icon sizes are usually less than or equal to 32x32.

The Windows SDK recommends that cursors and icons should be implemented as resources rather than created at run time.
We suggest using an icon with at least 3 images: 16x16 32bpp, 32x32 32 bpp and 48x48 32 bpp.

On Motif, it only works with some window managers, like *mwm* and *gnome*.
Icon colors can have the BGCOLOR values, but it works better if it is at index 0.

Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate an image to a name.

### Affects

[IupDialog](../dlg/iup_dialog.md)

### See Also

[IupImage](../elem/iup_image.md)
