## IupClose

Ends the IUP toolkit and releases internal memory.
It will also automatically destroy all dialogs and all elements that have names.

### Parameters/Return

    void IupClose(void);

### Notes

This function is usually called by the application to match the call to **IupOpen**.

On Android and iOS it is a no-op; the host platform owns process teardown.

### See Also

[IupOpen](iup_open.md)
