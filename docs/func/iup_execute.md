## IupExecute

Runs the executable with the given parameters.

It is a non-synchronous operation, i.e., the function will return just after execute the command, and it will not wait for its result.

In Windows, there is no need to add the ".exe" file extension.

On iOS, **filename** must be a URL (the platform forbids launching arbitrary executables).
On Android, **filename** can be a URL or an installed app's package name.
In WebAssembly, **filename** must be an http/https URL, opened in a new browser tab.

Used by the [IupHelp](iup_help.md) function.

### Parameters/Return

    int IupExecute(const char* filename, const char* parameters);

**filename**: name of the executable. Can contain a path.\
**parameters**: optional parameters. Can be NULL.

**Returns:** 1 if successful, -1 if failed. In Windows and GTK can return -2 if a file is not found.
