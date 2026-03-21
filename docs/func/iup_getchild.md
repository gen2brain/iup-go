## IupGetChild

Returns a child of the control given its position.

### Parameters/Return

    Ihandle *IupGetChild(Ihandle* ih, int pos);

**ih**: identifier of the interface element.\
**pos**: position of the desire child starting at 0.\

**Returns:** the child or NULL if there is none.

### Notes

This function will return the children of the control in the exact same order in which they were assigned.

### See Also

[IupGetChildPos](iup_getchildpos.md), [IupGetNextChild](iup_getnextchild.md), [IupGetBrother](iup_getbrother.md), [IupGetParent](iup_getparent.md)
