## IupImageFromHandle

Creates an **IupImage** from a native image handle.

### Parameters/Return

    Ihandle* IupImageFromHandle(void* handle);

**handle**: a native image handle, such as the **NATIVEIMAGE** returned by [IupClipboard](../elem/iup_clipboard.md).

**Returns:** handle of a new **IupImage**, or NULL if failed.

### Notes

The native handle type is driver-dependent (**GdkPixbuf***, a DIB **HANDLE**, **QPixmap***, and so on); see **NATIVEIMAGE** in [IupClipboard](../elem/iup_clipboard.md).

The pixel data is copied into the returned **IupImage**; the source handle is only read, so the caller keeps ownership of it.

### See Also

[IupImageGetHandle](iup_imagegethandle.md), [IupImage](../elem/iup_image.md), [IupClipboard](../elem/iup_clipboard.md)
