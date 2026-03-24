## IupImageGetHandle

Returns an **IupImage** handle from a name.

### Parameters/Return

    Ihandle* IupImageGetHandle(const char* name);

**name**: name of the image.

**Returns:** handle of the **IupImage** or NULL if failed.

### Notes

Name can be a global name set with **IupSetHandle**.
In this case, the function just returns the existing element.

Name can also be a file name that will be loaded from disk.
The available file formats supported are system-dependent:
- **Windows (Win32)**: BMP via LoadImage, plus BMP, GIF, JPEG, PNG, TIFF and others via WIC (Windows Imaging Component)
- **Windows (WinUI)**: BMP, GIF, JPEG, PNG, TIFF and others via WIC
- **GTK 3**: formats supported by GDK-PixBuf, such as BMP, GIF, JPEG, PNG, TIFF and many others
- **GTK 4**: formats supported by GdkTexture, such as PNG, JPEG, TIFF and others via GdkPixbuf
- **macOS**: formats supported by NSImage, such as BMP, GIF, JPEG, PNG, TIFF, PDF, EPS and others
- **Qt**: formats supported by QPixmap, such as BMP, GIF, JPEG, PNG, PBM, PGM, PPM, XBM, XPM and SVG
- **EFL**: formats supported by Evas image loaders, such as BMP, GIF, JPEG, PNG, TIFF, WebP and others
- **Motif**: X-Windows bitmap

In this case, the function returns a new image handle and associates the name with that handle, so in the next call it will return the existing handle.

Name can also be the name of a resource image defined in a RC file in Windows.
In this case, the function returns a new image handle and associates the name with that handle, so in the next call it will return the existing handle.

### See Also

[IupImage](../elem/iup_image.md), [IupImageSave](iup_imagesave.md), [IupImageSaveToBuffer](iup_imagesavetobuffer.md)
