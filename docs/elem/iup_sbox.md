## IupSbox

Creates a void container that allows its child to be resized.
Allows expanding and contracting the child **size** in one direction.

It does not have a native representation, but it also contains an **IupSeparator** to implement the bar handler.

### Creation

    Ihandle* IupSbox(Ihandle* child);

**child**: Identifier of an interface element which will receive the box. It can be NULL.

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**BARSIZE** (non-inheritable): controls the size of the bar handler. Default: "5".

**COLOR** (non-inheritable): Changes the color of the bar handler.
Default: "160 160 160".

**DIRECTION** (creation-only) (non-inheritable): Indicates the direction of the resize and the position of the bar handler.
Possible values are "NORTH", "SOUTH" (vertical direction), "EAST" or "WEST" (horizontal direction).
Default: "EAST".

[EXPAND](../attrib/iup_expand.md) (non-inheritable): It will expand automatically only in the direction opposite to the handler.

**LAYOUTDRAG** (non-inheritable): When the bar is moved, the children layout is automatically updated.
Default: "YES". If set to NO then the layout will be updated only when the mouse drag is released.

**SHOWGRIP** (non-inheritable): Shows the bar grip affordance. Default: "NO".
The grip is drawn only while the cursor is over the bar.
When set to NO, COLOR is used to fill the grip area.
If set to "LINES" then instead of the traditional grip appearance, it will be two parallel lines.

**WID** (read-only): returns -1 if mapped.

>
>
> ------------------------------------------------------------------------

[FONT](../attrib/iup_font.md), [SIZE](../attrib/iup_size.md), [RASTERSIZE](../attrib/iup_rastersize.md), [CLIENTSIZE](../attrib/iup_clientsize.md), [CLIENTOFFSET](../attrib/iup_clientoffset.md), [POSITION](../attrib/iup_position.md), [MINSIZE](../attrib/iup_minsize.md), [MAXSIZE](../attrib/iup_maxsize.md), [THEME](../attrib/iup_theme.md): also accepted.

### Notes

The controls to be resized must have the EXPAND=YES attribute set.
The control inside the sbox will not be resized to a size smaller than its natural size.
See the [Layout Guide](../layout.md#layout-guide) for more details on sizes.

The **IupSeparator** bar handler is always the first child of the sbox.
It can be obtained using **IupGetChild** or **IupGetNextChild**.

**IupSbox** can make the layout larger than the dialog size, so some controls will be positioned outside the dialog area at right or bottom.
This is part of the dynamic layout default reposition of controls inside the dialog.
See the **IupRefresh** function. The IUP layout does not have a maximum limit, only a minimum, except when the MAXSIZE common attribute is used.

The box can be created with no elements and be dynamically filled using [IupAppend](../func/iup_append.md) or [IupInsert](../func/iup_insert.md).

### Examples

[Browse for Example Files](../../examples/)

|                                 |                               |                                |                                |
|---------------------------------|-------------------------------|--------------------------------|--------------------------------|
| GTK                             | Qt                            | Win32                          | macOS                          |
| ![](../images/iupsbox_gtk3.png) | ![](../images/iupsbox_qt.png) | ![](../images/iupsbox_win.png) | ![](../images/iupsbox_mac.png) |

