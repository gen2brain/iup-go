## IupInsert

Inserts an interface element **before** another child of the container.
Valid for any element that contains other elements like dialog, frame, hbox, vbox, zbox, menu, etc.

### Parameters/Return

    Ihandle* IupInsert(Ihandle* ih, Ihandle* ref_child, Ihandle* new_child);

**ih**: Identifier of a container like hbox, vbox, zbox and menu.\
**ref_child**: Identifier of the element to be used as reference.
Can be NULL to insert as the first element.\
**new_child**: Identifier of the element to be inserted before the reference.

**Returns:** the actual **parent** if the interface element was successfully inserted.
Otherwise, returns NULL. Notice that the desired parent can contain a set of elements and containers where the child will be actually attached so the function returns the actual parent of the element.

### Notes

This function can be used when elements that will compose a container are not known *a priori* and should be dynamically constructed.

The new child cannot be mapped. It will NOT map the new child into the native system.
If the parent is already mapped, you must explicitly call **IupMap** for the appended child.

If the actual parent is a layout box (**IupVbox**, **IupHbox** or **IupZbox**) and you try to insert a child that it is already at the parent child list, then the child is moved to the insert position.

The elements are NOT immediately repositioned.
Call **IupRefresh** for the container* to update the dialog layout (* or any other element in the dialog).

### See Also

[IupAppend](iup_append.md), [IupDetach](iup_detach.md), [IupHbox](../elem/iup_hbox.md), [IupVbox](../elem/iup_vbox.md), [IupZbox](../elem/iup_zbox.md), [IupMenu](../elem/iup_menu.md), [IupMap](iup_map.md), [IupUnmap](iup_unmap.md), [IupRefresh](iup_refresh.md)
