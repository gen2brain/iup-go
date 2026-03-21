## CURSOR (non-inheritable) 

Defines the element's cursor.

### Value

Name of a cursor.

It will check first for the following predefined names:

|        ![](images/win_logo.png)        |    ![](images/x-win_logo.gif)     | Name |
|:--------------------------------------:|:---------------------------------:|----|
|                                        |                                   | "NONE" or "NULL" |
|  ![](images/wcursor_appstarting.gif)   |                ---                | "APPSTARTING" (Windows Only) |
|     ![](images/wcursor_arrow.gif)      |   ![](images/xcursor_arrow.gif)   | "ARROW" |
|      ![](images/wcursor_busy.gif)      |   ![](images/xcursor_busy.gif)    | "BUSY" |
|     ![](images/wcursor_cross.gif)      |   ![](images/xcursor_cross.gif)   | "CROSS" |
|      ![](images/wcursor_hand.gif)      |   ![](images/xcursor_hand.gif)    | "HAND" |
|      ![](images/wcursor_help.gif)      |   ![](images/xcursor_help.gif)    | "HELP" |
|      ![](images/wcursor_move.gif)      |   ![](images/xcursor_move.gif)    | "MOVE" |
|       ![](images/wcursor_no.gif)       |                ---                | "NO" (Windows and macOS) |
|   ![](images/wcursor_resize_ns.gif)    | ![](images/xcursor_resize_n.gif)  | "RESIZE_N" |
|   ![](images/wcursor_resize_ns.gif)    | ![](images/xcursor_resize_s.gif)  | "RESIZE_S" |
|   ![](images/wcursor_resize_ns.gif)    | ![](images/xcursor_resize_ns.gif) | "RESIZE_NS" |
|   ![](images/wcursor_resize_we.gif)    | ![](images/xcursor_resize_w.gif)  | "RESIZE_W" |
|   ![](images/wcursor_resize_we.gif)    | ![](images/xcursor_resize_e.gif)  | "RESIZE_E" |
|   ![](images/wcursor_resize_we.gif)    | ![](images/xcursor_resize_we.gif) | "RESIZE_WE" |
|  ![](images/wcursor_resize_nesw.gif)   | ![](images/xcursor_resize_ne.gif) | "RESIZE_NE" |
|  ![](images/wcursor_resize_nesw.gif)   | ![](images/xcursor_resize_sw.gif) | "RESIZE_SW" |
|  ![](images/wcursor_resize_nwse.gif)   | ![](images/xcursor_resize_nw.gif) | "RESIZE_NW" |
|  ![](images/wcursor_resize_nwse.gif)   | ![](images/xcursor_resize_se.gif) | "RESIZE_SE" |
| ![](images/wcursor_splitter_horiz.gif) | ![](images/xcursor_resize_we.gif) | "SPLITTER_HORIZ" |
| ![](images/wcursor_splitter_vert.gif)  | ![](images/xcursor_resize_ns.gif) | "SPLITTER_VERT" |
|      ![](images/wcursor_text.gif)      |   ![](images/xcursor_text.gif)    | "TEXT" |
|    ![](images/wcursor_uparrow.gif)     |  ![](images/xcursor_uparrow.gif)  | "UPARROW" |

Default: "ARROW"

The GTK cursors have the same appearance of the X-Windows cursors.
In Qt, the predefined names map to the standard Qt::CursorShape values.
In macOS, the predefined names map to NSCursor class methods.

If it is not a pre-defined name, then will check for other system cursors.
In Windows, the value will be used to load a cursor from the application resources.
In Motif, the value will be used as a X-Windows cursor number, see definitions in the X11 header "cursorfont.h".
In GTK, the value will be used as a cursor name, see the GDK documentation on Cursors.
In Qt, the value will be used to try to load the cursor from the system theme.

If no system cursors were found, then the value will be used to try to find an IUP image with the same name.
Use **IupSetHandle** to define a name for an **IupImage**.
But the image will need an extra attribute and some specific characteristics, see notes below.

### Notes

For an image to represent a cursor, it should have the attribute "**HOTSPOT"** to define the cursor hotspot (place where the mouse click is actually effective).
The default value is "0:0".

Usually only color indices 0, 1 and 2 can be used in a cursor, where 0 will be transparent (must be "BGCOLOR").
The RGB colors corresponding to indices 1 and 2 are defined just as in regular images.
In Windows, GTK, macOS and Qt, the cursor can have more than 2 colors and support RGBA images.
Cursor sizes are usually less than or equal to 32x32.

The cursor will only change when the interface system regains control or when IupFlush is called.

The Windows SDK recommends that cursors and icons should be implemented as resources rather than created at run time.

When the cursor image is no longer necessary, it must be destroyed through function [IupDestroy](../func/iup_destroy.md).
Attention: the cursor cannot be in use when it is destroyed.

### Affects

[IupDialog](../dlg/iup_dialog.md), [IupCanvas](../elem/iup_canvas.md)

### See Also

[IupImage](../elem/iup_image.md)
