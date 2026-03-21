## IupOpen

Initializes the IUP toolkit. Must be called before any other IUP function.

### Parameters/Return

    int IupOpen(int *argc, char ***argv);

**argc** and **argv**: are the same as the application "main" function.
Some parameters processed by the driver can be removed so the address is necessary.
They can be NULL.

**Returns:** IUP_OPENED (already opened), IUP_ERROR or IUP_NOERROR.

### Notes

The initialization is driver-dependent:

In **Windows (Win32)**, CoInitializeEx(COINIT_APARTMENTTHREADED) and InitCommonControlsEx are called.

In **Windows (WinUI)**, the WinRT apartment is initialized, a DispatcherQueue is created on the current thread, the XAML application object and WindowsXamlManager are initialized for XAML Islands support, and WinUI 3 control resources are loaded.
Returns IUP_ERROR if the WinUI bootstrap fails.

In **GTK 3**, gtk_init is called with argc/argv.
Returns IUP_ERROR if GTK initialization fails (e.g., no display available).

In **GTK 4**, gtk_init is called (without argc/argv, as GTK 4 no longer uses them).

In **Motif**, XtToolkitInitialize and XtOpenDisplay are called.
Returns IUP_ERROR if the X display cannot be opened.

In **macOS**, an NSAutoreleasePool is created, the NSApplication shared instance is initialized, and finishLaunching is called.

In **Qt**, a QApplication is created (or an existing instance is reused if one was already created by the application).
Returns IUP_ERROR if a QApplication cannot be obtained.

In **EFL**, elm_init is called with argc/argv.

In all drivers, the C numeric locale is reset to "C" after toolkit initialization to ensure consistent number formatting.

#### Environment Variables

The toolkit's initialization depends also on platform-dependent environment variables, see each driver documentation.

**QUIET**

When this variable is set to NO, IUP will generate a message in the console indicating the driver's version when initializing.
Default: YES.

**VERSION**

When this variable is set to YES, IUP generates a message dialog indicating the driver's version when initializing.
Default: NO.

### See Also

[IupClose](iup_close.md)
