## IupLog

Writes a message to the system log.

In Windows (Win32), writes to the "**Application**" event log via ReportEvent.
Except if type is DEBUG, in this case uses **OutputDebugString** to write to the current debugger.
If the application has no debugger and the system debugger is not active, it does nothing.
When running from Visual Studio, the message is displayed in the Output panel only when debugging the application.
To view the other messages, run the **Event Viewer** management console in Administrative Tools.

In Windows (WinUI), uses **OutputDebugString** for DEBUG, and writes to stdout/stderr for other types.

In Linux/Unix (GTK 3, GTK 4, Motif, EFL), writes to the **Syslog** via vsyslog.
When type is DEBUG, it will also write to the calling process' **Standard Error** stream.
If fails to submit a message to Syslog writes the message instead to system console.
To view the messages in Ubuntu, use the **File Log Viewer** application, or you can directly read the content of the /var/log/syslog file.

In macOS, writes to the Apple **Unified Logging** system via os_log.
Messages can be viewed using the **Console** application or the `log` command-line tool.

In Qt, uses the Qt logging system (qDebug, qInfo, qWarning, qCritical) via qt_message_output.
Messages go to the platform's default Qt logging output.

### Parameters/Return

    void IupLog(const char* type, const char* format, ...);
    void IupLogV(const char* type, const char* format, va_list arglist);

**type**: type of the log. Can be DEBUG, INFO, ERROR and WARNING.\
**format**: uses the same format specification as the **sprintf** function in C.
