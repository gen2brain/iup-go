## BGCOLOR

Element’s background color.

### Value

The RGB or RGBA components, in the format "R G B A".

Values should be between 0 and 255, separated by a blank space.
For example, "255 0 128", red=255 blue=0 green=128.

Alpha is optional and assumed to be 255 if not specified.
It is used only by the controls drawn with [IupDraw](../func/iup_draw.md), such as IupGauge, IupDial and the IupFlat* controls, with the driver limits listed there.

**Default**: It is the value of the DLGBGCOLOR or TXTBGCOLOR global attributes.
TXTBGCOLOR is used on IupText, IupList, and IupTree (Usually is "255 255 255" - white.).
On some controls if not defined will inherit the background of the native parent.

Hexadecimal notation in the format "#RRGGBB" is also accepted in all color attributes.
For example, "255 0 128" can also be written as "#FF0080".

### Affects

All controls that have visual representation, but with some restrictions.

Several controls have transparent parts that are not affected by the BGCOLOR.

### See Also

[FGCOLOR](iup_fgcolor.md), [DLGBGCOLOR](iup_globals.md#dlgbgcolor)
