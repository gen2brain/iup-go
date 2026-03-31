## IupScrollbar

Creates a Scrollbar control. Allows the user to scroll through a range of values by moving a thumb indicator.
Unlike the [SCROLLBAR](../attrib/iup_scrollbar.md) attribute used by IupCanvas, this is a standalone control.

### Creation

    Ihandle* IupScrollbar(const char *orientation);

**orientation**: optional orientation of the scrollbar. Can be NULL. See ORIENTATION attribute.

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

[BGCOLOR](../attrib/iup_bgcolor.md): Background color of the scrollbar.
Default: the global attribute DLGBGCOLOR.

**INVERTED** (creation-only) (non-inheritable): Invert the minimum and maximum positions on screen.
Default: NO.

**LINESTEP**: Controls the increment for keyboard arrows and single step clicks.
It is not the size of the increment.
The increment size is "linestep*(max-min)", so it must be 0<linestep<1.
Default is "0.01".

**MAX**: Contains the maximum scrollbar value. Default is "1".

**MIN**: Contains the minimum scrollbar value. Default is "0".

**ORIENTATION** (creation-only) (non-inheritable): Informs whether the scrollbar is "VERTICAL" or "HORIZONTAL".
Default: "HORIZONTAL".

**PAGESIZE**: The size of the visible page relative to the total range.
It defines the size of the thumb indicator.
The value is in the same units as MIN and MAX.
Default is "0.1".

**PAGESTEP**: Controls the increment for PgDn and PgUp keys and page step clicks.
It is not the size of the increment.
The increment size is "pagestep*(max-min)", so it must be 0<pagestep<1.
Default is "0.1".

[RASTERSIZE](../attrib/iup_rastersize.md) (non-inheritable): The initial size depends on the orientation.
For horizontal, the default width is 100 pixels and the height is the system scrollbar size.
For vertical, the default height is 100 pixels and the width is the system scrollbar size.

**TYPE**: Same as ORIENTATION. An alias maintained for compatibility.

**VALUE** (non-inheritable): Contains a number between MIN and MAX, indicating the scrollbar position.
Default: "0".

>
>
> ------------------------------------------------------------------------

[ACTIVE](../attrib/iup_active.md), [EXPAND](../attrib/iup_expand.md), [FONT](../attrib/iup_font.md), [SCREENPOSITION](../attrib/iup_screenposition.md), [POSITION](../attrib/iup_position.md), [MINSIZE](../attrib/iup_minsize.md), [MAXSIZE](../attrib/iup_maxsize.md), [WID](../attrib/iup_wid.md), [TIP](../attrib/iup_tip.md), [SIZE](../attrib/iup_size.md), [ZORDER](../attrib/iup_zorder.md), [VISIBLE](../attrib/iup_visible.md), [THEME](../attrib/iup_theme.md): also accepted.

### Callbacks

**VALUECHANGED_CB**: Called after the value was interactively changed by the user.

    int function(Ihandle *ih);

**ih**: identifier of the element that activated the event.

[SCROLL_CB](../call/iup_scroll_cb.md): Called when the user manipulates the scrollbar.
Provides the scroll operation code and position.

    int function(Ihandle *ih, int op, float posx, float posy);

**ih**: identifier of the element that activated the event.\
**op**: indicates the operation performed on the scrollbar.

If the scrollbar is vertical, it can have the following values:

    IUP_SBUP - line up
    IUP_SBDN - line down
    IUP_SBPGUP - page up
    IUP_SBPGDN - page down
    IUP_SBPOSV - vertical positioning
    IUP_SBDRAGV - vertical drag

If it is horizontal, the following values are valid:

    IUP_SBLEFT - column left
    IUP_SBRIGHT - column right
    IUP_SBPGLEFT - page left
    IUP_SBPGRIGHT - page right
    IUP_SBPOSH - horizontal positioning
    IUP_SBDRAGH - horizontal drag

**posx**, **posy**: scrollbar position (only the one matching the orientation is meaningful).

>
>
> ------------------------------------------------------------------------

[MAP_CB](../call/iup_map_cb.md), [UNMAP_CB](../call/iup_unmap_cb.md), [DESTROY_CB](../call/iup_destroy_cb.md), [GETFOCUS_CB](../call/iup_getfocus_cb.md), [KILLFOCUS_CB](../call/iup_killfocus_cb.md), [ENTERWINDOW_CB](../call/iup_enterwindow_cb.md), [LEAVEWINDOW_CB](../call/iup_leavewindow_cb.md), [K_ANY](../call/iup_k_any.md), [HELP_CB](../call/iup_help_cb.md): All common callbacks are supported.

### Notes

The range of the scrollbar is defined by MIN and MAX.
The PAGESIZE defines the size of the thumb indicator relative to the total range.
The actual usable range for VALUE is from MIN to MAX-PAGESIZE.

In GTK uses GtkScrollbar, in Windows uses the SCROLLBAR window class, in WinUI uses XAML ScrollBar, in macOS uses NSScroller, in Qt uses QScrollBar, in FLTK uses Fl_Scrollbar, in EFL uses Efl_Ui_Slider, and in Motif uses XmScrollBar.

### See Also

[IupVal](iup_val.md), [IupCanvas SCROLLBAR](../attrib/iup_scrollbar.md)
