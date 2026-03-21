## IupMap

Creates (**maps**) the native interface objects corresponding to the given IUP interface elements.

It will also call recursively to create the native element of all the children in the element's tree.

The element must be already **attached** to a mapped container, except the dialog.
A child can only be mapped if its parent is already mapped.

This function is automatically called before the dialog is shown in **IupShow**, **IupShowXY** or **IupPopup**. 

### Parameters/Return

    int IupMap(Ihandle* ih);

**ih**: Identifier of an interface element.

**Returns:** IUP_NOERROR if successful. If the element was already mapped returns IUP_NOERROR.
If the native creation failed returns IUP_ERROR.

### Notes

If the element is a dialog, then the abstract layout will be updated even if the dialog is already mapped.
If the dialog is visible, the elements will be immediately repositioned.
Calling **IupMap** for an already mapped dialog is the same as only calling **IupRefresh** for the dialog.

Calling **IupMap** for an already mapped element that is not a dialog does nothing.

If you add new elements to an already mapped dialog, you must call **IupMap** for those elements.
And then call **IupRefresh** to update the dialog layout.

If the WID attribute of an element is NULL, it means the element was not already mapped.
Some containers do not have a native element associated, like VBOX and HBOX.
In this case, their WID is a fake value (void*)(-1).

It is useful for the application to call **IupMap** when the value of the WID attribute must be known, i.e., the native element must exist, before a dialog is made visible.

The MAP_CB callback is called at the end of the **IupMap** function, after all processing, so it can also be used to create other things that depend on the WID attribute.
But notice that for non-dialog elements it will be called before the dialog layout has been updated, so the element current size will still be 0x0.

### See Also

[IupAppend](iup_append.md), [IupDetach](iup_detach.md), [IupUnmap](iup_unmap.md), [IupCreate](iup_create.md), [IupDestroy](iup_destroy.md), [IupShowXY](iup_showxy.md), [IupShow](iup_show.md), [IupPopup](iup_popup.md), [MAP_CB](../call/iup_map_cb.md)
