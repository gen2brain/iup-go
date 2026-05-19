## WID (read-only) (non-inheritable)

Element identifier in the native interface system.

### Value

The native handle type depends on the driver:

- **Windows (Win32 and WinUI)**: returns the **HWND** handle.
- **GTK 2, GTK 3 and GTK 4**: returns the **GtkWidget*** handle.
- **Motif**: returns the **Widget** handle.
- **macOS**: returns the **NSView*** or **NSWindow*** handle (as void*).
- **iOS**: returns the **UIView*** or **UIViewController*** handle (as void*).
- **Qt**: returns the **QWidget*** handle.
- **FLTK**: returns the **Fl_Widget*** handle.
- **EFL**: returns the **Evas_Object*** (Eo*) handle.
- **Android**: returns a JNI **GlobalRef** to the Java widget (as void*).
- **Haiku**: returns the **BView*** handle for controls, **BWindow*** for dialogs (as void*).

### Notes

Verification-only attribute, available after the control is mapped.

For elements that do not have a native representation, NULL is returned.

### Affects

All.
