## IupExecuteWait

Runs the executable with the given parameters.

It is a synchronous operation, i.e., the function will wait the command to terminate before it returns.

In Windows, there is no need to add the ".exe" file extension.

On iOS, behaves the same as [IupExecute](iup_execute.md); the platform exposes no way to wait for another app to exit.
On Android, returns -1; intents are fire-and-forget.

### Parameters/Return

    int IupExecuteWait(const char* filename, const char* parameters);

**filename**: name of the executable. Can contain a path.\
**parameters**: optional parameters. Can be NULL.

**Returns:** 1 if successful, -1 if failed. In Windows and GTK can return -2 if a file is not found.
