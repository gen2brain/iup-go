## FGCOLOR

Element’s foreground color. Usually it is the color of the associated text.

### Value

The RGB or RGBA components, in the format "R G B A".

Values should be between 0 and 255, separated by a blank space.
For example, "255 0 128", red=255 blue=0 green=128.

Alpha is optional and assumed to be 255 if not specified.
But it is only supported in custom controls drawn by IUP, example IupGauge, IupDial, all IupFlat* controls, and only when using OpenGL, Cairo or Direct2D draw drivers.
It is never supported when using X11, GDI or GDK draw drivers.

**Default**: It is the value of the DLGFGCOLOR or TXTFGCOLOR global attribute.
TXTFGCOLOR is used on IupText, IupList, IupTree and IupScintilla.  Usually is "0 0 0" - black.

Hexadecimal notation in the format "#RRGGBB" is also accepted in all color attributes.
For example, "255 0 128" can also be written as "#FF0080".

### Affects

All controls that have visual representation.

### See Also

[BGCOLOR](iup_bgcolor.md)
