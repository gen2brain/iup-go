## IupExecute

Runs the executable with the given parameters.

It is a non-synchronous operation, i.e., the function will return just after execute the command, and it will not wait for its result.

In Windows, there is no need to add the ".exe" file extension.

Used by the [IupHelp](iup_help.md) function.

### Parameters/Return

    int IupExecute(const char* filename, const char* parameters);

**filename**: name of the executable. Can contain a path.\
**parameters**: optional parameters. Can be NULL.

**Returns:** 1 if successful, -1 if failed. In Windows and GTK can return -2 if a file is not found.
