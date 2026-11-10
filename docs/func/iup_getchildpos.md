## IupGetChildPos

Returns the position of a child of the given control.

### Parameters/Return

    int IupGetChildPos(Ihandle* ih, Ihandle* child);

**ih**: identifier of the interface element.

**Returns:** the position of the desired child starting at 0, or -1 if the child is not found.

### Notes

The position follows the exact same order in which the children were assigned.

### See Also

[IupGetChild](iup_getchild.md), [IupGetChildCount](iup_getchildcount.md), [IupGetNextChild](iup_getnextchild.md), [IupGetBrother](iup_getbrother.md), [IupGetParent](iup_getparent.md)
