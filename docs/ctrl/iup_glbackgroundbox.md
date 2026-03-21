## IupGLBackgroundBox

Creates a simple native container with no decorations, but with OpenGL enabled.
It inherits from [IupGLCanvas](iup_glcanvas.md).

OBS: this is identical to the IupBackgroundBox element, but with OpenGL enabled.

### Creation

    Ihandle* IupGLBackgroundBox(Ihandle* child);

**child**: Identifier of an interface element which will receive the box. It can be NULL.

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

Inherits all attributes and callbacks of the [IupGLCanvas](iup_glcanvas.md), but redefines a few attributes.

[BGCOLOR](../attrib/iup_bgcolor.md): by default, will use the background color of the native parent, but can be set to a custom value.

**BORDER** (creation-only): the default value is "NO".

**CANVASBOX** (non-inheritable): enable the behavior of a canvas box instead of a regular container.
This will affect the EXPAND attribute, the Natural size computation, and child layout distribution.
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

**CANFOCUS**

(non-inheritable): the default is changed to NO. But it can receive the focus.

> 
>
> ------------------------------------------------------------------------

[CLIENTSIZE](../attrib/iup_clientsize.md), [CLIENTOFFSET](../attrib/iup_clientoffset.md): also accepted.

### Notes

The box can be created with no elements and be dynamic filled using [IupAppend](../func/iup_append.md) or [IupInsert](../func/iup_insert.md).

The ACTION callback can be defined, and the application can draw below other children inside the GLBackgroundBox, but only with the OpenGL API.
To avoid overlapping the drawing areas, it is recommended that children must be inside native containers.

### Examples

[Browse for Example Files](../../examples/)

