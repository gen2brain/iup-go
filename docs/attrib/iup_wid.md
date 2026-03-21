## WID (read-only) (non-inheritable)

Element identifier in the native interface system.

### Value

The native handle type depends on the driver:

- **Windows (Win32 and WinUI)**: returns the **HWND** handle.
- **GTK 3 and GTK 4**: returns the **GtkWidget*** handle.
- **Motif**: returns the **Widget** handle.
- **macOS**: returns the **NSView*** or **NSWindow*** handle (as void*).
- **Qt**: returns the **QWidget*** handle.
- **EFL**: returns the **Evas_Object*** (Eo*) handle.

### Notes

Verification-only attribute, available after the control is mapped.

For elements that do not have a native representation, NULL is returned.

### Affects

All.
