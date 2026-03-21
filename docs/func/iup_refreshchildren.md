## IupRefreshChildren

Updates the size and layout of controls after changing size attributes, or attributes that affect the size of the control.
Can be used for any element inside a dialog, only its children will be updated.
It can change the layout of all the controls inside the given element because of the dynamic layout positioning.

### Parameters/Return

    void IupRefreshChildren(Ihandle *ih);

**ih**: identifier of the interface element.

### Notes

The given element must be a container. It must be inside a dialog hierarchy.
It cannot be a dialog, for dialogs use **IupRefresh**.

The children are immediately repositioned, if the dialog is visible, then the change will be immediately reflected on the display.

This function will NOT change the size of the given element, even if the natural size of its children increases its natural size.

If your dialog has too many controls, and you want to hide or destroy some, then add some other in the same place, so you actually know that the dialog layout would not change, this is a much faster function than **IupRefresh**.

### See Also

[IupRefresh](iup_refresh.md)
