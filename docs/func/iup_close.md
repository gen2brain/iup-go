## IupClose

Ends the IUP toolkit and releases internal memory.
It will also automatically destroy all dialogs and all elements that have names.

### Parameters/Return

    void IupClose(void);

### Notes

In Windows, the CoUninitialize function is called.

In Motif, the XtDestroyApplicationContext function is called.

This function is usually called by the application to match the call to **IupOpen**.

### See Also

[IupOpen](iup_open.md)
