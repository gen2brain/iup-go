## IupRefresh

Updates the size and layout of all controls in the same dialog.

To be used after changing size attributes, or attributes that affect the size of the control.
Can be used for any element inside a dialog, but the layout of the dialog and all controls will be updated.
It can change the layout of all the controls inside the dialog because of the dynamic layout positioning.

### Parameters/Return

    void IupRefresh(Ihandle *ih);

**ih**: identifier of the interface element.

### Notes

Can be used for any control, but it will always affect the whole dialog.
Can be called even if the dialog is not mapped.

To refresh the layout of only a subset of the dialog use [IupRefreshChildren](iup_refreshchildren.md).

After the layout is computed, the position and size attributes are all updated.
If the elements are mapped, then they are immediately repositioned. If the dialog is visible, then the change will be immediately reflected on the display.

This function will NOT change the size of the dialog, **except** if the SIZE or RASTERSIZE attributes of the dialog where changed before the call.
For instance, if you also want to change the size of the dialog, then you can do:

    IupSetAttribute(dialog, "SIZE", ...);
    IupRefresh(dialog);

So the dialog will be resized for the new **User** size, if the new size is NULL, the dialog will be resized to the **Natural** size that includes all the elements.

Changing the size of elements without changing the dialog size may position some controls outside the dialog area at the left or bottom borders (the elements will be cropped at the dialog borders by the native system).

**IupMap** also updates the dialog layout, but only when called for the dialog itself, even if the dialog is already mapped.
Since **IupShow**, **IupShowXY** and **IupPopup** call **IupMap**, then they all will always update the dialog layout before showing it, even also if the dialog is already visible.

### See Also

[SIZE](../attrib/iup_size.md), [IupMap](iup_map.md), [IupRefreshChildren](iup_refreshchildren.md)
