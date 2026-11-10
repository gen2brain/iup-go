## IupClipboard

Creates an element that allows access to the clipboard.
Each clipboard should be destroyed using [IupDestroy](../func/iup_destroy.md), but a single one can serve the entire application because it does not store any data inside.
It can also be created and destroyed for every copy or paste.

### Creation

    Ihandle* IupClipboard(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**ADDFORMAT** (write-only): register a custom format for clipboard data given its name.
The registration remains valid even after the element is destroyed.
A new format must be added before used.
Custom format attributes (ADDFORMAT, FORMAT, FORMATAVAILABLE, FORMATDATA, FORMATDATASTRING, FORMATDATASIZE) are not supported in FLTK and WebAssembly.
In Android, only the string path is available (ADDFORMAT, FORMAT, FORMATAVAILABLE, FORMATDATASTRING, FORMATDATASIZE); the binary FORMATDATA is not.

**EMFAVAILABLE** [Windows Only] (read-only): informs if there is a Windows Enhanced Metafile available at the clipboard.

**FORMAT**: set the current format to be used by the FORMATAVAILABLE and FORMATDATA attributes.
This is a custom format string. The application copy and paste functions must know what it is copying and pasting in FORMATDATA based on that string.

**FORMATAVAILABLE** (read-only): informs if there is data in the FORMAT available at the clipboard.
If FORMAT is not set returns NULL.

**FORMATDATA**: sets or retrieves the data from the clipboard in the format defined by the FORMAT attribute.
If FORMAT is not set returns NULL. If set to NULL clears the clipboard data.
When set the FORMATDATASIZE attribute must be set before with the data size.
When retrieved FORMATDATASIZE will be set and available after data is retrieved.
Not supported in FLTK and Android.

**FORMATDATASTRING**: sets/gets FORMATDATA and FORMATDATASIZE considering data being a string in the system format.
Not supported in FLTK.

**FORMATDATASIZE**: size of the data on the clipboard.
Used by the FORMATDATA attribute processing.

**HTML**: copy or paste HTML formatted text to or from the clipboard. If set to NULL clears the clipboard data.
Only supported in Qt and QML.

**HTMLAVAILABLE** (read-only): informs if there is HTML formatted text available at the clipboard.
Only supported in Qt and QML.

**IMAGE** (write-only): name of an image to copy to the clipboard.
If set to NULL clears the clipboard data.

**IMAGEAVAILABLE** (read-only): informs if there is an image available at the clipboard.

**NATIVEIMAGE**: native handle of an image to copy or paste, to or from the clipboard.
In Win32 and WinUI, it is a **HANDLE** of a DIB.
In GTK, it is a **GdkPixbuf***.
In GTK 4, it is a **GdkTexture***.
In Motif, it is a **Pixmap**.
In macOS, it is an **NSImage***.
In iOS, it is a **UIImage***.
In Qt and QML, it is a **QPixmap***.
In FLTK, it is a **Fl_RGB_Image***.
In EFL, it is an **Evas_Object*** of an image.
In Haiku, it is a **BBitmap***.
In Android, it is an **android.graphics.Bitmap** global reference.
If set to NULL clears the clipboard data.
The returned handle in a paste must be released after use.
After copy, do NOT release the given handle.
Not supported in WebAssembly.

**NATIVEVECTORIMAGE**: copy or paste vector image data, a PDF document, to or from the clipboard.
When set the FORMATDATASIZE attribute must be set before with the data size.
When retrieved FORMATDATASIZE will be set and available after data is retrieved.
If set to NULL clears the clipboard data.
Supported in macOS, iOS, Qt and QML.

**PDFAVAILABLE** (read-only): informs if there is PDF vector image data available at the clipboard.
Supported in macOS, iOS, Qt and QML.

**SAVEEMF** [Windows Only] (write-only): saves the EMF from the clipboard to the given filename.
Available in Win32 and WinUI.

**SAVENATIVEVECTORIMAGE** (write-only): saves the PDF vector image from the clipboard to the given filename.
Supported in macOS, iOS, Qt and QML.

**SAVEWMF** [Windows Only] (write-only): saves the WMF from the clipboard to the given filename.
Available in Win32 and WinUI.

**SELECTION**: selects the buffer used by the other attributes. Can be CLIPBOARD or PRIMARY. Default: CLIPBOARD.
PRIMARY is the X11 primary selection, pasted with the middle mouse button.
Supported in GTK, GTK 4, Qt, QML, Motif, FLTK and EFL.
In FLTK, it affects only TEXT and TEXTAVAILABLE, and only in X11.

**TEXT**: copy or paste text to or from the clipboard. If set to NULL clears the clipboard data.

**TEXTAVAILABLE** (read-only): informs if there is a text available at the clipboard.

**WMFAVAILABLE** [Windows Only] (read-only): informs if there is a Windows Metafile available at the clipboard.

### Notes

In Windows, when "TEXT" format data is copied to the clipboard, the system will automatically store other text formats too if those formats are not already stored.
This means that when copying "TEXT" Windows will also store "Unicode Text" and "OEM Text", but only if those formats were not copied before.
To make sure the system will copy all the other text formats, clear the clipboard before copying the data (set TEXT=NULL before setting the actual value).

### Examples

    Ihandle* clipboard = IupClipboard();
    IupSetAttribute(clipboard, "TEXT", IupGetAttribute(text, "VALUE"));
    IupDestroy(clipboard);

    Ihandle* clipboard = IupClipboard();
    IupSetAttribute(text, "VALUE", IupGetAttribute(clipboard, "TEXT"));
    IupDestroy(clipboard);
