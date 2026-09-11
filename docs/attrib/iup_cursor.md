## CURSOR (non-inheritable) 

Defines the element's cursor.

### Value

Name of a cursor.

It will check first for the following predefined names:

|        ![](../images/win_logo.png)        |    ![](../images/x-win_logo.gif)     |                 macOS                 | Name                                    |
|:-----------------------------------------:|:------------------------------------:|:-------------------------------------:|-----------------------------------------|
|                                           |                                      |                                       | "NONE" or "NULL"                        |
|  ![](../images/wcursor_appstarting.gif)   |                 ---                  |                  ---                  | "APPSTARTING" (Win32, WinUI Only)       |
|     ![](../images/wcursor_arrow.gif)      |   ![](../images/xcursor_arrow.gif)   | ![](../images/mcursor_arrow.png) | "ARROW"                                 |
|      ![](../images/wcursor_busy.gif)      |   ![](../images/xcursor_busy.gif)    | ![](../images/mcursor_busy.png) | "BUSY"                                  |
|     ![](../images/wcursor_cross.gif)      |   ![](../images/xcursor_cross.gif)   | ![](../images/mcursor_cross.png) | "CROSS"                                 |
|      ![](../images/wcursor_hand.gif)      |   ![](../images/xcursor_hand.gif)    | ![](../images/mcursor_hand.png) | "HAND"                                  |
|      ![](../images/wcursor_help.gif)      |   ![](../images/xcursor_help.gif)    | ![](../images/mcursor_help.png) | "HELP"                                  |
|      ![](../images/wcursor_move.gif)      |   ![](../images/xcursor_move.gif)    | ![](../images/mcursor_move.png) | "MOVE"                                  |
|       ![](../images/wcursor_no.gif)       |                 ---                  | ![](../images/mcursor_no.png) | "NO" (not in GTK, Qt, Motif, FLTK, EFL) |
|                    ---                    |                 ---                  |                  ---                  | "PEN" (not in Cocoa)                    |
|   ![](../images/wcursor_resize_ns.gif)    | ![](../images/xcursor_resize_n.gif)  | ![](../images/mcursor_resize_n.png) | "RESIZE_N"                              |
|   ![](../images/wcursor_resize_ns.gif)    | ![](../images/xcursor_resize_s.gif)  | ![](../images/mcursor_resize_s.png) | "RESIZE_S"                              |
|   ![](../images/wcursor_resize_ns.gif)    | ![](../images/xcursor_resize_ns.gif) | ![](../images/mcursor_resize_ns.png) | "RESIZE_NS"                             |
|   ![](../images/wcursor_resize_we.gif)    | ![](../images/xcursor_resize_w.gif)  | ![](../images/mcursor_resize_w.png) | "RESIZE_W"                              |
|   ![](../images/wcursor_resize_we.gif)    | ![](../images/xcursor_resize_e.gif)  | ![](../images/mcursor_resize_e.png) | "RESIZE_E"                              |
|   ![](../images/wcursor_resize_we.gif)    | ![](../images/xcursor_resize_we.gif) | ![](../images/mcursor_resize_we.png) | "RESIZE_WE"                             |
|  ![](../images/wcursor_resize_nesw.gif)   | ![](../images/xcursor_resize_ne.gif) | ![](../images/mcursor_resize_ne.png) | "RESIZE_NE"                             |
|  ![](../images/wcursor_resize_nesw.gif)   | ![](../images/xcursor_resize_sw.gif) | ![](../images/mcursor_resize_sw.png) | "RESIZE_SW"                             |
|  ![](../images/wcursor_resize_nwse.gif)   | ![](../images/xcursor_resize_nw.gif) | ![](../images/mcursor_resize_nw.png) | "RESIZE_NW"                             |
|  ![](../images/wcursor_resize_nwse.gif)   | ![](../images/xcursor_resize_se.gif) | ![](../images/mcursor_resize_se.png) | "RESIZE_SE"                             |
| ![](../images/wcursor_splitter_horiz.gif) | ![](../images/xcursor_resize_we.gif) | ![](../images/mcursor_splitter_horiz.png) | "SPLITTER_HORIZ"                        |
| ![](../images/wcursor_splitter_vert.gif)  | ![](../images/xcursor_resize_ns.gif) | ![](../images/mcursor_splitter_vert.png) | "SPLITTER_VERT"                         |
|      ![](../images/wcursor_text.gif)      |   ![](../images/xcursor_text.gif)    | ![](../images/mcursor_text.png) | "TEXT"                                  |
|    ![](../images/wcursor_uparrow.gif)     |  ![](../images/xcursor_uparrow.gif)  | ![](../images/mcursor_uparrow.png) | "UPARROW"                               |

Default: "ARROW"

If it is not a pre-defined name, drivers that map cursors via system-theme names (GTK, GTK4, Qt, Motif, EFL) try the platform's cursor theme. Win32 tries the application resources. Motif also accepts an X-Windows cursor number from `cursorfont.h` (or an Xcursor file on Motif 2.4.0+).

In macOS, the RESIZE and SPLITTER names use the system frame, row and column resize cursors on macOS 15 and later. HELP shows the contextual menu cursor and UPARROW the arrow.

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

Not supported on Android and iOS (touch UIs have no cursor).

The Windows SDK recommends that cursors and icons should be implemented as resources rather than created at run time.

When the cursor image is no longer necessary, it must be destroyed through function [IupDestroy](../func/iup_destroy.md).
Attention: the cursor cannot be in use when it is destroyed.

### Affects

[IupDialog](../dlg/iup_dialog.md), [IupCanvas](../elem/iup_canvas.md)

### See Also

[IupImage](../elem/iup_image.md)
