## IupBackgroundBox

Creates a simple native container with no decorations.
Useful for controlling children visibility for **IupZbox** or **IupExpander**.
It inherits from [IupCanvas](../elem/iup_canvas.md).

### Creation

    Ihandle* IupBackgroundBox(Ihandle* child);

**child**: Identifier of an interface element which will receive the box. It can be NULL.

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

Inherits all attributes and callbacks of the [IupCanvas](../elem/iup_canvas.md), but redefines a few attributes.

**BACKIMAGE** (non-inheritable): image name to be used as background.
Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate an image to a name.
See also [IupImage](../elem/iup_image.md).
When defined the ACTION callback of the IupCanvas will be defined.

**BACKIMAGEZOOM** (non-inheritable): if set the back image will be zoomed to occupy the full background.
Aspect ratio is NOT preserved. Can be Yes or No. Default: No.

**BACKCOLOR** (non-inheritable): if defined used to fill the background color when BACKIMAGE is defined.
If not defined BGCOLOR is used.

[BGCOLOR](../attrib/iup_bgcolor.md): by default, will use the background color of the native parent, but can be set to a custom value.

**BORDER** (creation-only): the default value is "NO".

**CANVASBOX** (non-inheritable): enable the behavior of a canvas box instead of a regular container.
This will affect the EXPAND attribute, the Natural size computation, and children layout distribution.
Can be Yes or No. Default: No.

**CHILDOFFSET** (non-inheritable): Allow to specify a position offset for the child.
Available for native containers only. It will not affect the natural size, and allows to position controls outside the client area.
Format "*dx*x*dy*", where *dx* and *dy* are integer values corresponding to the horizontal and vertical offsets, respectively, in pixels.
Default: 0x0.

**DECORATION** (non-inheritable): Enable a decoration area around the child. Can be Yes or No.
Default No.

**DECORSIZE** (non-inheritable): total size of the decoration in the format "WidthxHeight" (in C "%dx%d).
Used only when DECORATION=Yes.

**DECOROFFSET** (non-inheritable): decoration offset from a left border and top border in the format "XxY" (in C "%dx%d).
Used only when DECORATION=Yes.

[EXPAND](../attrib/iup_expand.md) (non-inheritable):  behaves as a container.
See CANVASBOX attribute.

**CANFOCUS**> (non-inheritable): the default is changed to NO.
But it can receive the focus.

> 
>
> ------------------------------------------------------------------------

[CLIENTSIZE](../attrib/iup_clientsize.md), [CLIENTOFFSET](../attrib/iup_clientoffset.md), [THEME](../attrib/iup_theme.md): also accepted.

### Notes

The box can be created with no elements and be dynamic filled using [IupAppend](../func/iup_append.md) or [IupInsert](../func/iup_insert.md).

The ACTION callback can be defined, and the application can draw below other children inside the BackgroundBox, but only with the [IupDraw](../func/iup_draw.md) API.
To avoid overlapping the drawing areas, it is recommended that children must be inside native containers.

### Examples

[Browse for Example Files](../../examples/)

