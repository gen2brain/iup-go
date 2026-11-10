## IupSplit

Creates a void container that splits its client area in two.
It allows the provided controls to be enclosed in a box that allows expanding and contracting the element **size** in one direction, but when one is expanded the other is contracted.

It does not have a native representation, but it also contains an **IupSeparator** to implement the bar handler.

### Creation

    Ihandle* IupSplit(Ihandle* child1, Ihandle* child2);

**child1**: Identifier of an interface element that will be at left or top. It can be NULL.\
**child2**: Identifier of an interface element that will be at right or bottom. It can be NULL.

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**AUTOHIDE** (non-inheritable): if the child client area is smaller than the bar size, then the child is automatically hidden.
Default: "NO".

**BARSIZE** (non-inheritable): controls the size of the bar handler. Default: "5".

**COLOR** (non-inheritable): Changes the color of the bar grip affordance. Default: "160 160 160" ("90 90 90" when DARKMODE is enabled).

**ORIENTATION** (creation-only) (non-inheritable): Indicates the orientation of the bar handler.
The direction of the resize is perpendicular to the orientation. Possible values are "VERTICAL" or "HORIZONTAL".
Default: "VERTICAL".

[EXPAND](../attrib/iup_expand.md) (non-inheritable): Default: "YES".

**LAYOUTDRAG** (non-inheritable): When the bar is moved, the children layout is automatically updated.
Default: "YES". If set to NO then the layout will be updated only when the mouse drag is released.

**MINMAX** (non-inheritable): sets minimum and maximum crop values for VALUE, in the format "%d:%d" [min:max].
The min value cannot be less than 0, and the max value cannot be larger than 1000.
This will constrain the interactive change of the bar handler. Default: "0:1000".

**SHOWGRIP** (non-inheritable): Shows the bar grip affordance. Default: "YES". When set to NO, the BARSIZE is set to 3.
The grip is drawn only while the cursor is over the bar.
When set to NO, COLOR is used to fill the grip area if defined, if COLOR is not defined the area is filled with the parent background color.
If set to "LINES" then instead of the traditional grip appearance, it will be two parallel lines.

**VALUE** (non-inheritable): The proportion of the left or top (child1) client area relative to the full available area.
It is an integer between 0 and 1000. If not defined or set to NULL, the natural size of the two children will define its initial size.

**WID** (read-only): returns -1 if mapped.

>
>
> ------------------------------------------------------------------------

[FONT](../attrib/iup_font.md), [SIZE](../attrib/iup_size.md), [RASTERSIZE](../attrib/iup_rastersize.md), [CLIENTSIZE](../attrib/iup_clientsize.md), [CLIENTOFFSET](../attrib/iup_clientoffset.md), [POSITION](../attrib/iup_position.md), [MINSIZE](../attrib/iup_minsize.md), [MAXSIZE](../attrib/iup_maxsize.md), [THEME](../attrib/iup_theme.md): also accepted.

### Callbacks

**VALUECHANGED_CB**: Called after the value was interactively changed by the user.

    int function(Ihandle *ih);

**ih**: identifier of the element that activated the event.

### Notes

The controls to be resized must have the EXPAND=YES attribute set.
See the [Layout Guide](../layout.md#layout-guide) for more details on sizes.

When the MINSIZE or MAXSIZE attribute is set for a direct child, **IupSplit** respects that size.
Nested children will also have their size limits respected.

The **IupSeparator** bar handler is always the first child of the split.
It can be obtained using **IupGetChild** or **IupGetNextChild**.

The **IupSplit** control looks just like the **IupSbox**, but internally is different.
While the **IupSbox** controls only one element and can push other elements outside the dialog, the **IupSplit** balances its internal size and never pushes other elements outside its boundaries.

When AUTOHIDE=YES, the control will set FLOATING=IGNORE and VISIBLE=NO for the child to be auto-hidden, then back to FLOATING=NO and VISIBLE=YES when shown.
If the child has several children with different combinations of VISIBLE, this child should be a native container like **IupBackgroundBox** or **IupFrame**, so the VISIBLE attribute will not be propagated to its children.

The container can be created with no elements and be dynamically filled using [IupAppend](../func/iup_append.md) or [IupInsert](../func/iup_insert.md).

### Examples

[Browse for Example Files](../../examples/)

|                                  |                                |                                 |                                 |
|----------------------------------|--------------------------------|---------------------------------|---------------------------------|
| GTK                              | Qt                             | Win32                           | macOS                           |
| ![](../images/iupsplit_gtk3.png) | ![](../images/iupsplit_qt.png) | ![](../images/iupsplit_win.png) | ![](../images/iupsplit_mac.png) |

