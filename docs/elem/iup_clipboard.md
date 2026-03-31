## IupClipboard

Creates an element that allows access to the clipboard.
Each clipboard should be destroyed using [IupDestroy](../func/iup_destroy.md), but you can use only one for the entire application because it does not store any data inside.
Or you can simply create and destroy every time you need to copy or paste.

### Creation

    Ihandle* IupClipboard(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**ADDFORMAT** (write-only): register a custom format for clipboard data given its name.
The registration remains valid even after the element is destroyed.
A new format must be added before used.
Custom format attributes (ADDFORMAT, FORMAT, FORMATAVAILABLE, FORMATDATA, FORMATDATASTRING, FORMATDATASIZE) are not supported in FLTK.

**EMFAVAILABLE** (read-only) [Windows Only]: informs if there is a Windows Enhanced Metafile available at the clipboard.

**FORMAT**: set the current format to be used by the FORMATAVAILABLE and FORMATDATA attributes.
This is a custom format string. The application copy and paste functions must know what it is copying and pasting in FORMATDATA based on that string.

**FORMATAVAILABLE** (read-only): informs if there is data in the FORMAT available at the clipboard.
If FORMAT is not set returns NULL.

**FORMATDATA**: sets or retrieves the data from the clipboard in the format defined by the FORMAT attribute.
If FORMAT is not set returns NULL. If set to NULL clears the clipboard data.
When set the FORMATDATASIZE attribute must be set before with the data size.
When retrieved FORMATDATASIZE will be set and available after data is retrieved.

**FORMATDATASTRING**: sets/gets FORMATDATA and FORMATDATASIZE considering data being a string in the system format.
Not supported in Motif and FLTK.

**FORMATDATASIZE**: size of the data on the clipboard.
Used by the FORMATDATA attribute processing.

**IMAGE** (write-only): name of an image to copy to the clipboard.
If set to NULL clears the clipboard data.
Not supported in EFL.

**IMAGEAVAILABLE** (read-only): informs if there is an image available at the clipboard.
Not supported in EFL.

**NATIVEIMAGE**: native handle of an image to copy or paste, to or from the clipboard.
In Win32 and WinUI is a **HANDLE** of a DIB.
In GTK is a **GdkPixbuf***.
In GTK 4 is a **GdkTexture***.
In Motif is a **Pixmap**.
In macOS is an **NSImage***.
In Qt is a **QPixmap***.
In FLTK is a **Fl_RGB_Image***.
If set to NULL clears the clipboard data.
The returned handle in a paste must be released after used.
After copy, do NOT release the given handle.
Not supported in EFL.

**SAVEEMF** (write-only) [Windows Only]: saves the EMF from the clipboard to the given filename.
Available in Win32 and WinUI.

**SAVEWMF** (write-only) [Windows Only]: saves the WMF from the clipboard to the given filename.
Available in Win32 and WinUI.

**TEXT**: copy or paste text to or from the clipboard. If set to NULL clears the clipboard data.

**TEXTAVAILABLE** (read-only): informs if there is a text available at the clipboard.

**WMFAVAILABLE** (read-only) [Windows Only]: informs if there is a Windows Metafile available at the clipboard.

### Notes

In Windows when "TEXT" format data is copied to the clipboard, the system will automatically store other text formats too if those formats are not already stored.
This means that when copying "TEXT" Windows will also store "Unicode Text" and "OEM Text", but only if those formats were not copied before.
So to make sure the system will copy all the other text formats clear the clipboard before copying your own data (you can simply set TEXT=NULL before setting the actual value).

### Examples

    Ihandle* clipboard = IupClipboard();
    IupSetAttribute(clipboard, "TEXT", IupGetAttribute(text, "VALUE"));
    IupDestroy(clipboard);

    Ihandle* clipboard = IupClipboard();
    IupSetAttribute(text, "VALUE", IupGetAttribute(clipboard, "TEXT"));
    IupDestroy(clipboard);
