## GETFOCUS_CB

Action generated when an element is given keyboard focus.
This callback is called after the KILLFOCUS_CB of the element that lost the focus.
The IupGetFocus function during the callback returns the element that lost the focus.

### Callback

    int function(Ihandle *ih);

**ih**: identifier of the element that received keyboard focus.

### Affects

All elements with user interaction, except menus.

### See Also

[KILLFOCUS_CB](iup_killfocus_cb.md), [IupGetFocus](../func/iup_getfocus.md), [IupSetFocus](../func/iup_setfocus.md)
