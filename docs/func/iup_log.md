## IupLog

Writes a message to the system log.

In Windows, writes to the "**Application**" event log.
Except if type is DEBUG, in this case uses **OutputDebugString** to write to the current debugger.
If the application has no debugger and the system debugger is not active, it does nothing.
When running from Visual Studio, the message is displayed in the Output panel only when debugging the application.
To view the other messages, run the **Event Viewer** management console in Administrative Tools.

In Linux, write to the **Syslog**. When type is DEBUG, it will also write to the calling process' **Standard Error** stream.
If fails to submit a message to Syslog writes the message instead to system console.
To view the messages in Ubuntu, use the **File Log Viewer** application, or you can directly read the content of the /var/log/syslog file.
It is common to grab the last lines of the file using "tail -n 10 /var/log/syslog".
You can also filter messages using grep and the filename of the application.

### Parameters/Return

    void IupLog(const char* type, const char* format, ...);
    void IupLogV(const char* type, const char* format, va_list arglist);

**type**: type of the log. Can be DEBUG, INFO, ERROR and WARNING.\
**format**: uses the same format specification as the **sprintf** function in C.
 
