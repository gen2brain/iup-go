## ENTRY_POINT

Global callback for an entry point. Used when main is not possible, such as in iOS and Android systems.

It is called only once, when the main loop is processed, but after **IupOpen**.
For regular systems is called right before the actual event loop is started.

### Callback

    void function(void);

### Notes

It can only be set using **IupSetFunction(**name, func**)**.

### See Also

[IupSetFunction](../func/iup_setfunction.md), [EXIT_CB](iup_exit_cb.md), [IupMainLoopLevel](../func/iup_mainlooplevel.md) 
