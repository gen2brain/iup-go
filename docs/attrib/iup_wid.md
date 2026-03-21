## WID (read-only) (non-inheritable)

Element identifier in the native interface system.

### Value

In Motif, returns the **Widget** handle.

In Windows, returns the **HWND** handle.

In GTK, return the **GtkWidget*** handle.

### Notes

Verification-only attribute, available after the control is mapped.

For elements that do not have a native representation, NULL is returned.

### Affects

All.
