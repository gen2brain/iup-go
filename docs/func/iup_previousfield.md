## IupPreviousField

Shifts the focus to the previous element that can have the focus.
It is relative to the given element and does not depend on the element currently with the focus.

### Parameters/Return

    Ihandle* IupPreviousField(Ihandle* ih);

**ih**: identifier of the interface element.

**Returns:** the element that received the focus or NULL if not found.

### See Also

[IupNextField](iup_nextfield.md).
