## IupHelp

Opens the given URL in the default application.

The URL is dispatched to the system's default handler.
On Linux/Unix the browser can be overridden via the IUP_HELPAPP environment variable or the global attribute HELPAPP.

It is a non-synchronous operation, i.e., the function will return just after executing the command, and it will not wait for its result.

It will use the [IupExecute](iup_execute.md) function.

### Parameters/Return

    int IupHelp(const char* url);

**url**: it may be any kind of address accepted by the Browser, that is, it can include 'http://', or be just a file name, etc.

**Returns:** 1 if successful, -1 if failed. In Windows can return -2 if a file is not found.
