## KILLFOCUS_CB

Action generated when an element loses keyboard focus.
This callback is called before the GETFOCUS_CB of the element that gets the focus.

### Callback

    int function(Ihandle *ih);

**ih**: identifier of the element that activated the event.

### Affects

All elements with user interaction, except menus.

In Windows, there are restrictions when using this callback.
From MSDN on WM_KILLFOCUS: "“While processing this message, do not make any function calls that display or activate a window.
This causes the thread to yield control and can cause the application to stop responding to messages.”

### See Also

[GETFOCUS_CB](iup_getfocus_cb.md), [IupGetFocus](../func/iup_getfocus.md), [IupSetFocus](../func/iup_setfocus.md)
