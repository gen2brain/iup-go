## IupSetFocus

Sets the interface element that will receive the keyboard focus, i.e., the element that will receive keyboard events.
This is processed only after the control actually receives the focus.

### Parameters/Return

    Ihandle *IupSetFocus(Ihandle *ih);

**ih**: identifier of the interface element that will receive the keyboard focus.
Only elements that can have the keyboard focus and are mapped, active and visible can be used; other elements are ignored.

**Returns:** the identifier of the interface element that previously had the keyboard focus.

### Notes

The value returned by **IupGetFocus** will be updated only after the main loop regains the control and the control actually receives the focus.
When **IupGetFocus** is called right after **IupSetFocus**, the return value will be different.
Calling **IupFlush** between the two functions obtains the same value in both calls.

### See Also

[IupGetFocus](iup_getfocus.md).
