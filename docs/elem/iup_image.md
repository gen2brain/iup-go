## IupImage, IupImageRGB, IupImageRGBA

Creates an image to be shown on a label, button, toggle, or as a cursor.



### Creation

    Ihandle* IupImage(int width, int height, const unsigned char *pixels);
    Ihandle* IupImageRGB(int width, int height, const unsigned char *pixels);
    Ihandle* IupImageRGBA(int width, int height, const unsigned char *pixels);



**width**: Image width in pixels.\
**height**: Image height in pixels.\
**pixels**: Vector containing the value of each pixel.
**IupImage** uses 1 value per pixel, **IupImageRGB** uses 3 values and  **IupImageRGBA** uses 4 values per pixel.
Each value is always 8 bit. Origin is at the top-left corner, and data is oriented top to bottom, and left to right.
The pixels array is duplicated internally so you can discard it after the call.\
**pixel0**, **pixel1**, **pixel2**, ...: Value of the pixels.
But for **IupImageRGB** and **IupImageRGBA** in fact will be one value for each color channel (pixel_r_0, pixel_g_0, pixel_b_0, pixel_r_1, pixel_g_1, pixel_b_1, pixel_r_2, pixel_g_2, pixel_b_2, ...).\
**line0**, **line1**: unnamed tables, one for each line containing pixels values. See Notes below.\
**colors**: table named colors containing the colors indices.

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**"0"** Color in index 0.\
**"1"** Color in index 1.\
**"2"** Color in index 2.\
**...**\
**"i"** Color in index i.

> The indices can range from 0 to 255. The total number of colors is limited to 256 colors.
>
> The values are integer numbers from 0 to 255, one for each color in the RGB triple (For ex: "64 190 255"). If the value of a given index is "BGCOLOR", the color used will be the background color of the element on which the image will be inserted. The "BGCOLOR" value must be defined within an index less than 16.
>
> Used only for images created with **IupImage**.

**AUTOSCALE**: automatically scale the image by a given real factor. Can be "DPI" or a scale factor.
If not defined the global attribute [IMAGEAUTOSCALE](../attrib/iup_globals.md) will be used.
Values are the same of the global attribute.
The minimum resulted size when automatically resized is 24 pixels height.

**BGCOLOR**: The color used for transparency.
If not defined uses the BGCOLOR of the control that contains the image.

**BPP** (read-only): returns the number of bits per pixel in the image.
Images created with **IupImage** returns 8, with **IupImageRGB** returns 24 and with **IupImageRGBA** returns 32.

**CLEARCACHE** (write-only): clears the internal native image cache, so WID can be dynamically changed.

**CHANNELS** (read-only): returns the number of channels in the image.
Images created with **IupImage** returns 1, with **IupImageRGB** returns 3 and with **IupImageRGBA** returns 4.

**DPI**: resolution expected for display. Used when AUTOSCALE=DPI.
If not defined the global attribute [IMAGESDPI](../attrib/iup_globals.md) will be used.

**HEIGHT** (read-only): Image height in pixels.

**HOTSPOT**: Hotspot is the position inside a cursor image indicating the mouse-click spot.
Its value is given by the x and y coordinates inside a cursor image.
Its value has the format "x:y", where x and y are integers defining the coordinates in pixels.
Default: "0:0".

**RASTERSIZE** (read-only): returns the image size in pixels.

**RESHAPE** (write-only): given a new size if format "*width*x*height*", allocates enough memory for the new size and changes WIDTH and HEIGHT attributes.
Image contents is ignored and it will contain trash after the reshape.

**RESIZE** (write-only): given a new size if format "*width*x*height*", changes WIDTH and HEIGHT attributes, and resizes the image contents using bilinear interpolation for RGB and RGBA images and nearest neighborhood for 8 bits.

**SCALED** (read-only): returns Yes if the image has been resized.

**ORIGINALSCALE** (read-only): returns the width and height before the image was scaled.

**WID** (read-only): returns the internal pixels data pointer.

**WIDTH** (read-only): Image width in pixels.

### Notes

Application icons are usually 32x32. Toolbar bitmaps are 24x24 or smaller.
Menu bitmaps and small icons are 16x16 or smaller.

Images created with the **IupImage*** constructors can be reused in different elements.

The images should be destroyed when they are no longer necessary, by means of the **IupDestroy** function.
To destroy an image, it cannot be in use, i.e., the controls where it is used should be destroyed first.
Images that were associated with controls by names are automatically destroyed in **IupClose**.

Please observe the rules for creating cursor images: [CURSOR](../attrib/iup_cursor.md).

In GTK uses GdkPixbuf/GdkCursor, in GTK 4 uses GdkTexture/GdkCursor, in Windows uses HBITMAP/HICON, in WinUI uses WriteableBitmap, in macOS uses NSImage, in Qt uses QPixmap, in FLTK uses Fl_RGB_Image, in EFL uses Evas_Object, and in Motif uses Pixmap/Cursor.

#### Usage

Images are used in elements such as buttons and labels by attributes that points to names registered with [IupSetHandle](../func/iup_sethandle.md).
You can also use **IupSetAttributeHandle** to shortcut the set of an image as an attribute.
For example:

    Ihandle* image = IupImage(width, height, pixels);

    IupSetHandle("MY_IMAGE_NAME", image);
    IupSetAttribute(label, "IMAGE", "MY_IMAGE_NAME");
    or
    IupSetAttributeHandle(label, "IMAGE", image); // an automatic name will be created internally

In Windows, names of resources in RC files linked with the application are also accepted.
In GTK, names of GTK Stock Items are also accepted.
In GTK 4, names of icon theme icons are also accepted.
In Qt, names of Qt standard pixmaps (QStyle::StandardPixmap) are also accepted for stock icons.
In Motif, names of bitmaps installed on the system are also accepted. For example:

    IupSetAttribute(label, "IMAGE", "TECGRAF_BITMAP");  // available in the "etc/iup.rc" file
    or
    IupSetAttribute(label, "IMAGE", "gtk-open");  // available in the GTK Stock Items

In all drivers, a path to a file name can also be used as the attribute value.
The available file formats supported are system-dependent.
In Windows, BMP, ICO and CUR are supported via LoadImage, plus BMP, GIF, JPEG, PNG, TIFF and others via WIC.
In GTK, the formats supported by GDK-PixBuf are available, such as BMP, GIF, JPEG, PNG, TIFF and many others.
In macOS, the formats supported by NSImage are available, such as BMP, GIF, JPEG, PNG, TIFF and others.
In Qt, the formats supported by QPixmap are available, such as BMP, GIF, JPEG, PNG and others.
In EFL, the formats supported by Evas image loaders are available, such as BMP, GIF, JPEG, PNG, TIFF and others.
In Motif, the X-Windows bitmap format is supported. For example:

    IupSetAttribute(label, "IMAGE", "../etc/tecgraf.bmp");

#### Colors

In Motif, the alpha channel in RGBA images is always composed with the control BGCOLOR by IUP prior to setting the image at the control.
In Windows, GTK, GTK 4, macOS, Qt and EFL, the alpha channel is composed internally by the system.
But in Windows for some controls, the alpha must be composed a priori also, it includes: **IupItem** and **IupSubmenu** always; and **IupToggle** when NOT using Visual Styles.
This implies that if the control background is not uniform, then probably there will be a visible difference where it should be transparent.

For **IupImage**, if a color is not set, then it is used a default color for the 16 first colors.
The default color table is the same for all drivers:

     0 =   0,  0,  0 (black)
     1 = 128,  0,  0 (dark red)
     2 =   0,128,  0 (dark green) 
     3 = 128,128,  0 (dark yellow)
     4 =   0,  0,128 (dark blue)
     5 = 128,  0,128 (dark magenta) 
     6 =   0,128,128 (dark cian) 
     7 = 192,192,192 (gray)
     8 = 128,128,128 (dark gray)
     9 = 255,  0,  0 (red)     
    10 =   0,255,  0 (green)
    11 = 255,255,  0 (yellow)
    12 =   0,  0,255 (blue)
    13 = 255,  0,255 (magenta)
    14 =   0,255,255 (cian)  
    15 = 255,255,255 (white)

For images with more than 16 colors, and up to 256 colors, all the color indices must be defined up to the maximum number of colors.
For example, if the biggest image index is 100, then all the colors from i=16 up to i=100 must be defined even if some indices are not used.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupLabel](iup_label.md), [IupButton](iup_button.md), [IupToggle](iup_toggle.md), [IupDestroy](../func/iup_destroy.md), [IupImageGetHandle](../func/iup_imagegethandle.md), [IupImageSave](../func/iup_imagesave.md), [IupImageSaveToBuffer](../func/iup_imagesavetobuffer.md).
