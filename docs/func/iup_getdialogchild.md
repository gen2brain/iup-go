## IupGetDialogChild

Returns the identifier of the child element that has the NAME attribute equals to the given value on the same dialog hierarchy.
Works also for children of a menu that is associated with a dialog.

### Parameters/Return

    Ihandle* IupGetDialogChild(Ihandle *ih, const char* name);

**ih**: Identifier of an interface element that belongs to the hierarchy.\
**name**: name of the control to be found

**Returns:** NULL if not found.

### Notes

This function will only find the child if the NAME attribute is set at the control.

Before the dialog is mapped, the function searches the hierarchy, even if the hierarchy does not belong to a dialog yet, but after the child is mapped the result is immediate (the hierarchy is not searched).

### See Also

[NAME](../attrib/iup_name.md)
