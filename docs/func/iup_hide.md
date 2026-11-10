## IupHide

Hides an interface element. This function has the same effect as setting the interface element’s VISIBLE attribute to "NO".

### Parameters/Return

    int IupHide(Ihandle *ih);

**ih**: Identifier of the interface element.

**Returns:** IUP_NOERROR always.

### Notes

Once a dialog is hidden, either by means of **IupHide** or by changing the VISIBLE attribute or by means of a click in the window close button, the elements inside this dialog are not destroyed, so the dialog can be shown again.
To destroy dialogs, the **IupDestroy** function must be called.

### See Also

[IupShowXY](iup_showxy.md), [IupShow](iup_show.md), [IupPopup](iup_popup.md), [IupDestroy](iup_destroy.md).
