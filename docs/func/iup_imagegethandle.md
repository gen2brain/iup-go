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
But the available file formats supported are system-dependent. The Windows driver supports BMP.
The GTK driver supports the formats supported by the GDK-PixBuf library, such as BMP, GIF, JPEG, PCX, PNG, TIFF and many others.
The Motif driver supports the X-Windows bitmap.
In this case, the function returns a new image handle and associates the name with that handle, so in the next call it will return the existing handle.

Name can also be the name of a resource image defined in a RC file in Windows.
In this case, the function returns a new image handle and associates the name with that handle, so in the next call it will return the existing handle.

### See Also

[IupImage](../elem/iup_image.md)
