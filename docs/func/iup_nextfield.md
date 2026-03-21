## IupNextField

Shifts the focus to the next element that can have the focus.
It is relative to the given element and does not depend on the element currently with the focus.

It will search for the next element first in the children, then in the brothers, then in the uncles and their children, and so on.

This sequence is not the same sequence used by the Tab key, which is dependent on the native system.

### Parameters/Return

    Ihandle* IupNextField(Ihandle* ih);

**ih**: identifier of the interface element.

**Returns:** the element that received the focus or NULL if not found.

### See Also

[IupPreviousField](iup_previousfield.md).
