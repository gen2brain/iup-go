## EXIT_CB

Global callback for an exit. Used when main is not possible, such as in iOS and Android systems.

It is called every time the last main loop is ended.
For regular systems is called right after the actual event loop is ended, every time when the main loop level returns to 0.

### Callback

    void function(void);

### Notes

It can only be set using **IupSetFunction(**name, func**)**.

### See Also

[IupSetFunction](../func/iup_setfunction.md), [ENTRY_POINT](iup_entry_point.md),
