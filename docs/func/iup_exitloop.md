## IupExitLoop

Terminates the current message loop. It has the same effect as a callback returning IUP_CLOSE.

### Parameters/Return

    void IupExitLoop(void);

### Notes

In Android and iOS, there is no message loop to return from (see [IupMainLoop](iup_mainloop.md)); IupExitLoop closes the top dialog instead. In Android, closing the main dialog exits the app; in iOS, the main dialog cannot be dismissed (only presented sheets are).
