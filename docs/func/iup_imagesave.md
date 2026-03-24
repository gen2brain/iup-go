## IupImageSave

Saves an **IupImage** to a file using native driver image encoding.

### Parameters/Return

    int IupImageSave(Ihandle* ih, const char* filename, const char* format);

**ih**: handle of the image (created with IupImage, IupImageRGB, IupImageRGBA, or returned by IupDrawGetImage/IupImageGetHandle).

**filename**: file path to save to.

**format**: image format string: `"PNG"`, `"JPEG"`, or `"BMP"`. Can be NULL to auto-detect from the filename extension.

**Returns:** 1 on success, 0 on failure.

### Notes

The image is encoded using the native GUI toolkit, with no external library dependencies. Supported formats are platform-dependent:
- **Windows (Win32)**: PNG, JPEG, BMP via WIC (Windows Imaging Component)
- **Windows (WinUI)**: PNG, JPEG, BMP via WIC
- **GTK 3**: PNG, JPEG, BMP via GDK-PixBuf
- **GTK 4**: PNG, JPEG, BMP via GDK-PixBuf
- **macOS**: PNG, JPEG, BMP via NSBitmapImageRep
- **Qt**: PNG, JPEG, BMP via QImage
- **EFL**: PNG, JPEG, BMP via Evas image savers
- **Motif**: BMP only (pure C implementation)

If **format** is NULL, the format is detected from the filename extension (`.png`, `.jpg`/`.jpeg`, `.bmp`).

JPEG quality defaults to 85 and can be configured with `IupSetGlobal("IMAGESAVEQUALITY", "90")`, where the value is 0-100.

All image types are supported: 8-bit indexed (palette is expanded), 24-bit RGB, and 32-bit RGBA.

### See Also

[IupImageSaveToBuffer](iup_imagesavetobuffer.md), [IupImageGetHandle](iup_imagegethandle.md), [IupImage](../elem/iup_image.md), [IupDrawGetImage](iup_draw.md)
