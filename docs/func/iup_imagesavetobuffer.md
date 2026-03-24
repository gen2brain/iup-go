## IupImageSaveToBuffer

Encodes an **IupImage** to a memory buffer using native driver image encoding.

### Parameters/Return

    unsigned char* IupImageSaveToBuffer(Ihandle* ih, const char* format, int* size);

**ih**: handle of the image (created with IupImage, IupImageRGB, IupImageRGBA, or returned by IupDrawGetImage/IupImageGetHandle).

**format**: image format string: `"PNG"`, `"JPEG"`, or `"BMP"`. Cannot be NULL.

**size**: pointer to an integer that receives the buffer size in bytes.

**Returns:** pointer to a malloc-allocated buffer containing the encoded image data, or NULL on failure. The caller must free this buffer with `free()`.

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

Unlike **IupImageSave**, the **format** parameter is required since there is no filename to detect the format from.

JPEG quality defaults to 85 and can be configured with `IupSetGlobal("IMAGESAVEQUALITY", "90")`, where the value is 0-100.

All image types are supported: 8-bit indexed (palette is expanded), 24-bit RGB, and 32-bit RGBA.

### See Also

[IupImageSave](iup_imagesave.md), [IupImageGetHandle](iup_imagegethandle.md), [IupImage](../elem/iup_image.md), [IupDrawGetImage](iup_draw.md)
