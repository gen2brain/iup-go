## IupImageSaveToBuffer

Encodes an **IupImage** to a memory buffer using native driver image encoding.

### Parameters/Return

    unsigned char* IupImageSaveToBuffer(Ihandle* ih, const char* format, int* size);

**ih**: handle of the image (created with IupImage, IupImageRGB, IupImageRGBA, or returned by IupDrawGetImage/IupImageGetHandle).

**format**: image format string: `"PNG"`, `"JPEG"`, or `"BMP"`. Cannot be NULL.

**size**: pointer to an integer that receives the buffer size in bytes.

**Returns:** pointer to a malloc-allocated buffer containing the encoded image data, or NULL on failure. The caller must free this buffer with `free()`.

### Notes

Unlike **IupImageSave**, the **format** parameter is required since there is no filename to detect the format from.

See [IupImageSave](iup_imagesave.md) for the per-driver supported formats, JPEG quality control, and accepted image types.

### See Also

[IupImageSave](iup_imagesave.md), [IupImageGetHandle](iup_imagegethandle.md), [IupImage](../elem/iup_image.md), [IupDrawGetImage](iup_draw.md)
