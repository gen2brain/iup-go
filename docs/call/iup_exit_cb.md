## EXIT_CB

Global callback for an exit. Used when main is not possible, such as in iOS and Android systems.

It is called every time the last main loop is ended.
For regular systems is called right after the actual event loop is ended, every time when the main loop level returns to 0.
On Android it is called when the last activity is destroyed.
On iOS it is called only if the system terminates the app while it is still running; suspended apps are usually terminated without notification, so it is normally not called there.

### Callback

    void function(void);

### Notes

It can only be set using **IupSetFunction(**name, func**)**.

### See Also

[IupSetFunction](../func/iup_setfunction.md), [ENTRY_POINT](iup_entry_point.md),
