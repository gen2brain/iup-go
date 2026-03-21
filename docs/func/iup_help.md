## IupHelp

Opens the given URL in the default application.

In Windows, executes the shell "open" operation on the given URL via ShellExecute.

In Linux/Unix with GTK 3, Motif, or EFL, it first attempts to use the XDG Desktop Portal (org.freedesktop.portal.OpenURI) via D-Bus.
If the portal is not available, it falls back to the xdg-open command.

In Linux/Unix with GTK 4, it uses the GIO g_app_info_launch_default_for_uri function, falling back to xdg-open if that fails.

In Qt, it uses QDesktopServices::openUrl.

In macOS, it uses the "open" command.

In Linux/Unix, you can override the browser by setting the environment variable IUP_HELPAPP or using the global attribute "HELPAPP".

It is a non-synchronous operation, i.e., the function will return just after executing the command, and it will not wait for its result.

It will use the [IupExecute](iup_execute.md) function.

### Parameters/Return

    int IupHelp(const char* url);

**url**: it may be any kind of address accepted by the Browser, that is, it can include 'http://', or be just a file name, etc.

**Returns:** 1 if successful, -1 if failed. In Windows can return -2 if a file is not found.
