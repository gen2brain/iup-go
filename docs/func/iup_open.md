## IupOpen

Initializes the IUP toolkit. Must be called before any other IUP function.

### Parameters/Return

    int IupOpen(int *argc, char ***argv);

**argc** and **argv**: are the same as the application "main" function.
Some parameters processed by the driver can be removed so the address is necessary.
They can be NULL.

**Returns:** IUP_OPENED (already opened), IUP_ERROR or IUP_NOERROR.
Only in UNIX can fail to open, because X-Windows may be not initialized.

### Notes

In Windows, **CoInitializeEx(COINIT_APARTMENTTHREADED)** and **InitCommonControlsEx(ICC_WIN95_CLASSES)** functions are called.

In Motif, **XtOpenApplication** function is called.

#### Environment Variables

The toolkit's initialization depends also on platform-dependent environment variables, see each driver documentation.

**QUIET**

When this variable is set to NO, IUP will generate a message in the console indicating the driver’s version when initializing.
Default: YES.

**VERSION**

When this variable is set to YES, IUP generates a message dialog indicating the driver's version when initializing.  Default: NO.

### See Also

[IupClose](iup_close.md)
