## IupSetFocus

Sets the interface element that will receive the keyboard focus, i.e., the element that will receive keyboard events.
But this will be processed only after the control actually receives the focus.

### Parameters/Return

    Ihandle *IupSetFocus(Ihandle *ih);

**ih**: identifier of the interface element that will receive the keyboard focus.
Only elements that can have the keyboard focus are mapped, active and visible can be used, other elements are ignored.

**Returns:** the identifier of the interface element that previously had the keyboard focus.

### Notes

The value returned by **IupGetFocus** will be updated only after the main loop regains the control and the control actually receives the focus.
So if you call **IupGetFocus** right after **IupSetFocus** the return value will be different.
You could call **IupFlush** between the two functions to obtain the same value in both calls.

### See Also

[IupGetFocus](iup_getfocus.md).

 
