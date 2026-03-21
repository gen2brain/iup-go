## IupConvertXYToPos

Converts a (x,y) coordinate in an item position.

### Parameters/Return

    int IupConvertXYToPos(Ihandle *ih, int x, int y);

**ih**: Identifier of the element.\
**x**: X coordinate relative to the left corner of the element.\
**y**: Y coordinate relative to the top corner of the element.

**Returns:** the position starting at 0 (except for **IupList** that starts at 1).
If fails returns -1.

### Notes

It can be used for **IupText** and **IupScintilla** (returns a position in the string), **IupList** (returns an item), **IupTree** (returns a node identifier) or **IupMatrix** (returns a cell position, where pos=lin*numcol + col).

### See Also

[IupText](../elem/iup_text.md), [IupList](../elem/iup_list.md), [IupTree](../elem/iup_tree.md), [IupMatrix](../ctrl/iup_matrix.md)
