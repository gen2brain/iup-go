## IupLog

Writes a message to the system log.

- **Windows (Win32)**: Application event log via ReportEvent (view with **Event Viewer**). DEBUG type uses OutputDebugString instead.
- **Windows (WinUI)**: OutputDebugString for DEBUG, stdout/stderr for other types.
- **Linux/Unix (GTK 3, GTK 4, Motif, EFL, FLTK)**: Syslog via vsyslog (e.g. `/var/log/syslog`). DEBUG also writes to stderr.
- **macOS and iOS**: Apple Unified Logging via os_log (view with **Console** or `log`).
- **Qt**: qt_message_output (qDebug, qInfo, qWarning, qCritical).
- **Android**: logcat with tag "IupLog" (view with `adb logcat`).

### Parameters/Return

    void IupLog(const char* type, const char* format, ...);
    void IupLogV(const char* type, const char* format, va_list arglist);

**type**: type of the log. Can be DEBUG, INFO, ERROR and WARNING.\
**format**: uses the same format specification as the **sprintf** function in C.
