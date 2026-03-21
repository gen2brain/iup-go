## TIP (non-inheritable)

Text to be shown when the mouse lies over the element.

### Value

Text.

### Additional Tip Attributes

These attributes affect the TIP display.

**TIPBALLOON** [Windows Only]: The tip window will have the appearance of a cartoon "balloon" with rounded corners and a stem pointing to the item.
Default: NO.

**TIPBALLOONTITLE** [Windows Only]: When using the balloon format, the tip can also have a title in a separate area.

**TIPBALLOONTITLEICON** [Windows Only]: When using the balloon format, the tip can also have a pre-defined icon in the title area.
Values can be:

"0" - No icon (default)\
"1" - Info icon\
"2" - Warning icon\
"3" - Error Icon

**TIPBGCOLOR**: The tip background color.
Default: "255 255 225" (Light Yellow).
Supported in Windows, WinUI, Motif, Qt and EFL.

**TIPDELAY** [Windows and Motif Only]: Time the tip will remain visible. Default: "5000".
In Windows the maximum value is 32767 milliseconds.

**TIPFGCOLOR**: The tip text color. Default: "0 0 0" (Black).
Supported in Windows, WinUI, Motif, Qt and EFL.

**TIPFONT**: The font for the tip text.
If not defined the font used for the text is the same as the FONT attribute for the element.
If the value is SYSTEM then, no font is selected and the default system font for the tip will be used.
Supported in Windows, WinUI, Motif, Qt and EFL.

**TIPICON** [GTK and EFL Only]: name of an image to be displayed in the TIP.
See [IupImage](../elem/iup_image.md).

**TIPMARKUP** [GTK Only]: allows the tip string to contain Pango markup commands.
Can be "YES" or "NO". Default: "NO". Must be set before setting the TIP attribute.

**TIPRECT** (non-inheritable): Specifies a rectangle inside the element where the tip will be activated.
Format: "%d %d %d %d"="x1 y1 x2 y2". Default: all the element area.

**TIPVISIBLE**: Shows or hides the tip under the mouse cursor. Use values "YES" or "NO".
Returns the current visible state.

### Additional Tip Callbacks

**TIPS_CB**: Action before a tip is displayed.

    int function(Ihandle* ih, int x, int y);

**ih**: identifier of the element that activated the event.\
**x, y**: cursor position relative to the top-left corner of the element

### Affects

All controls that have visual representation, except menus.
