## IupHelp

Opens the given URL. In UNIX executes Netscape, Safari (macOS) or Firefox (in Linux) passing the desired URL as a parameter.
In Windows executes the shell "open" operation on the given URL.

In UNIX, you can change the used browser setting the environment variable IUP_HELPAPP or using the global attribute "HELPAPP".

It is a non-synchronous operation, i.e., the function will return just after executing the command, and it will not wait for its result.

It will use the [IupExecute](iup_execute.md) function.

### Parameters/Return

    int IupHelp(const char* url);

**url**: it may be any kind of address accepted by the Browser, that is, it can include 'http://', or be just a file name, etc.

**Returns:** 1 if successful, -1 if failed. In Windows can return -2 if a file is not found.
