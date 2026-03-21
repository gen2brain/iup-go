## IupGetChildPos

Returns the position of a child of the given control.

### Parameters/Return

    int IupGetChildPos(Ihandle* ih, Ihandle* child);

**ih**: identifier of the interface element.

**Returns:** the position of the desire child starting at 0, or -1 if child is not found.

### Notes

This function will return the children of the control in the exact same order in which they were assigned.

### See Also

[IupGetChild](iup_getchild.md), [IupGetChildCount](iup_getchildcount.md), [IupGetNextChild](iup_getnextchild.md), [IupGetBrother](iup_getbrother.md), [IupGetParent](iup_getparent.md)
