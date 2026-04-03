## IupPopover

Creates a Popover container. It is a floating container that displays content anchored to another element.
The popover is shown and hidden by setting the VISIBLE attribute.

### Creation

    Ihandle* IupPopover(Ihandle* child);

**child**: identifier of an interface element that will be displayed inside the popover. It can be NULL.

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**ANCHOR** (non-inheritable): The element the popover is anchored to.
Must be set before the popover is mapped.
Use [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate the anchor element.

**ARROW** (non-inheritable): Shows an arrow pointing to the anchor element.
Can be "YES" or "NO". Default: "YES".
Only supported in GTK 4 and macOS. In GTK 3 the arrow is always shown. In other systems the popover is displayed without an arrow.

**AUTOHIDE** (non-inheritable): When enabled, the popover is automatically hidden when the user clicks outside of it or when focus leaves.
Clicks on the anchor element do not trigger auto-hide.
Can be "YES" or "NO". Default: "YES".

**POSITION** (non-inheritable): The position of the popover relative to the anchor element.
Can be "BOTTOM", "TOP", "LEFT", "RIGHT", "BOTTOMLEFT", "BOTTOMRIGHT", "TOPLEFT", "TOPRIGHT", "LEFTBOTTOM", "LEFTTOP", "RIGHTBOTTOM" or "RIGHTTOP". Default: "BOTTOM".

The basic values (BOTTOM, TOP, LEFT, RIGHT) center the popover along the anchor edge. The compound values specify both the edge and the alignment: the first word is the edge where the popover appears, the second word is the alignment along that edge. For example, "BOTTOMLEFT" places the popover below the anchor with left edges aligned, and "RIGHTTOP" places it to the right with top edges aligned.

In WinUI all positions map directly to native FlyoutPlacementMode. In GTK 3 and GTK 4 the edge-aligned positions are approximated using the native popover positioning with offsets. In other systems the positions are calculated manually.

**OFFSETX** (non-inheritable): Horizontal pixel offset added to the computed popover position. Can be positive or negative. Default: "0".
Not supported in WinUI and macOS.

**OFFSETY** (non-inheritable): Vertical pixel offset added to the computed popover position. Can be positive or negative. Default: "0".
Not supported in WinUI and macOS.

[VISIBLE](../attrib/iup_visible.md) (non-inheritable): Shows or hides the popover.
The popover is mapped on the first time VISIBLE is set to "YES".
The ANCHOR element must be set and mapped before showing the popover.

>
>
> ------------------------------------------------------------------------

[ACTIVE](../attrib/iup_active.md), [FONT](../attrib/iup_font.md), [SCREENPOSITION](../attrib/iup_screenposition.md), [POSITION](../attrib/iup_position.md), [MINSIZE](../attrib/iup_minsize.md), [MAXSIZE](../attrib/iup_maxsize.md), [WID](../attrib/iup_wid.md), [TIP](../attrib/iup_tip.md), [SIZE](../attrib/iup_size.md), [RASTERSIZE](../attrib/iup_rastersize.md), [ZORDER](../attrib/iup_zorder.md), [THEME](../attrib/iup_theme.md): also accepted.

### Callbacks

**SHOW_CB**: Called when the popover is shown or hidden.

    int function(Ihandle *ih, int state);

**ih**: identifier of the element that activated the event.\
**state**: IUP_SHOW (1) when the popover becomes visible, IUP_HIDE (0) when it is hidden.

>
>
> ------------------------------------------------------------------------

[MAP_CB](../call/iup_map_cb.md), [UNMAP_CB](../call/iup_unmap_cb.md), [DESTROY_CB](../call/iup_destroy_cb.md), [K_ANY](../call/iup_k_any.md): common callbacks are supported.

### Notes

The popover uses the child's natural size to determine its own size.
It does not expand and accepts exactly one child element.

The ANCHOR attribute must be set before mapping, and the anchor element must already be mapped when the popover is first shown.

When AUTOHIDE=YES, the popover behaves like a modal popup that closes on outside interaction.
When AUTOHIDE=NO, the popover remains visible until explicitly hidden via VISIBLE=NO.

In GTK 3 and GTK 4 uses GtkPopover, in Windows uses a custom popup window, in WinUI uses XAML Flyout, in macOS uses NSPopover, in Qt uses a custom QFrame popup, in FLTK uses a borderless Fl_Window popup, in EFL uses a borderless popup window, and in Motif uses a transient shell popup.

### See Also

[IupDialog](../dlg/iup_dialog.md), [IupMenu](iup_menu.md)
