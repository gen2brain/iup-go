## IupTable

Creates a Table control with columns and rows for displaying and editing tabular data.
Unlike [IupMatrix](../ctrl/iup_matrix.md) which is custom-drawn, IupTable uses native table/list widgets on each platform.

### Creation

    Ihandle* IupTable(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

#### Dimensions

**NUMLIN** (non-inheritable): Number of data rows in the table.
Can be changed after creation to add or remove rows.

**NUMCOL** (non-inheritable): Number of columns in the table.
Can be changed after creation to add or remove columns.

**COUNT** (read-only) (non-inheritable): Returns the total number of cells (NUMLIN * NUMCOL).

#### Structure Operations

**ADDLIN** (write-only) (non-inheritable): Adds a new row at the given position.
Value is the 1-based line index where the row will be inserted.

**DELLIN** (write-only) (non-inheritable): Deletes the row at the given position.
Value is the 1-based line index to remove.

**ADDCOL** (write-only) (non-inheritable): Adds a new column at the given position.
Value is the 1-based column index where the column will be inserted.

**DELCOL** (write-only) (non-inheritable): Deletes the column at the given position.
Value is the 1-based column index to remove.

#### Cell Values

**IDVALUElin:col**: Gets or sets the text value of a cell.
Uses L:C notation where L is the 1-based line and C is the 1-based column.

**VALUE** (non-inheritable): Gets or sets the value of the currently focused cell.

#### Column Attributes (using column ID, 1-based)

**TITLEcol** (non-inheritable): Column header text.
n starts at 1.

**WIDTHcol** (non-inheritable): Column width in pixels.
n starts at 1.

**RASTERWIDTHcol** (non-inheritable): Same as WIDTHcol.

**ALIGNMENTcol** (non-inheritable): Column text alignment.
Can be "ALEFT", "ACENTER" or "ARIGHT". Default: "ALEFT".
n starts at 1.

#### Selection and Focus

**FOCUSCELL** (non-inheritable): Gets or sets the focused cell in "L:C" format.
Supports partial syntax: "2:" changes only the line, ":3" changes only the column.
Default: "1:1".

**SELECTIONMODE** (non-inheritable): Selection mode.
Can be "NONE", "SINGLE" or "MULTIPLE". Default: "SINGLE".

#### Display

**SHOWGRID** (non-inheritable): Shows grid lines between cells.
Can be "YES" or "NO". Default: "YES".

**FOCUSRECT** (non-inheritable): Draws a dashed focus rectangle around the focused cell.
Can be "YES" or "NO". Default: "YES".

**SHOW** (write-only): Scrolls the table to make the specified cell visible.
Value is in "L:C" format.

**REDRAW** (write-only): Forces a full table redraw.

#### Editing

**EDITABLE** (non-inheritable): Enables in-place editing for all cells.
Can be "YES" or "NO".

**EDITABLEcol** (non-inheritable): Enables in-place editing for a specific column.
n starts at 1.

#### Colors

[BGCOLOR](../attrib/iup_bgcolor.md): Background color.
Supports L:C notation for per-cell color, :C for per-column, L:* for per-row.

[FGCOLOR](../attrib/iup_fgcolor.md): Foreground text color.
Same L:C notation as BGCOLOR.

**ALTERNATECOLOR** (non-inheritable): Enables alternating row background colors.
Can be "YES" or "NO". Default: "NO".

**EVENROWCOLOR** (non-inheritable): Background color for even-numbered rows.
Only used when ALTERNATECOLOR=YES.

**ODDROWCOLOR** (non-inheritable): Background color for odd-numbered rows.
Only used when ALTERNATECOLOR=YES.

#### Behavior

**SORTABLE** (non-inheritable): Enables column sorting when the user clicks on a column header.
Can be "YES" or "NO". Default: "NO".
The application must handle the actual sorting via SORT_CB.

**ALLOWREORDER** (non-inheritable): Enables column reordering via drag-and-drop on the header.
Can be "YES" or "NO". Default: "NO".

**USERRESIZE** (non-inheritable): Enables user column resizing by dragging the header dividers.
Can be "YES" or "NO". Default: "NO".

**STRETCHLAST** (non-inheritable): The last column stretches to fill the remaining table width.
Can be "YES" or "NO". Default: "YES".

#### Virtual Mode

**VIRTUALMODE** (non-inheritable): Enables virtual mode for large datasets.
When enabled, cell values are not stored internally but retrieved on demand via VALUE_CB.
Can be "YES" or "NO". Default: "NO".
Must be set before the control is mapped.

#### Images

**SHOWIMAGE** (non-inheritable): Enables per-cell image display.
Can be "YES" or "NO". Default: "NO".
Must be set before the control is mapped.

**FITIMAGE** (non-inheritable): Scales images to fit the row height.
Can be "YES" or "NO". Default: "YES".

**IMAGElin:col** (write-only) (non-inheritable): Sets the image for a cell.
Uses L:C notation. The value is an image name set with [IupSetHandle](../func/iup_sethandle.md).

#### Natural Size

**VISIBLECOLUMNS**: Number of columns to consider for the natural size calculation.
When not set, all columns are used (capped at the actual column count).

**VISIBLELINES**: Number of data rows to consider for the natural size calculation.
When not set, uses the actual row count (default 10 if no rows, capped at 15).

>
>
> ------------------------------------------------------------------------

[ACTIVE](../attrib/iup_active.md), [EXPAND](../attrib/iup_expand.md), [FONT](../attrib/iup_font.md), [SCREENPOSITION](../attrib/iup_screenposition.md), [POSITION](../attrib/iup_position.md), [MINSIZE](../attrib/iup_minsize.md), [MAXSIZE](../attrib/iup_maxsize.md), [WID](../attrib/iup_wid.md), [TIP](../attrib/iup_tip.md), [SIZE](../attrib/iup_size.md), [RASTERSIZE](../attrib/iup_rastersize.md), [ZORDER](../attrib/iup_zorder.md), [VISIBLE](../attrib/iup_visible.md), [THEME](../attrib/iup_theme.md): also accepted.

The default value of EXPAND is "YES".

### Callbacks

**CLICK_CB**: Action generated when the user clicks on a cell.

    int function(Ihandle *ih, int lin, int col, char *status);

**ih**: identifier of the element that activated the event.\
**lin**: line number (1-based).\
**col**: column number (1-based).\
**status**: status of the mouse buttons and some keyboard keys at the moment the event is generated.

**Returns:** IUP_IGNORE to suppress default handling.

**ENTERITEM_CB**: Action generated when the focus cell changes.

    int function(Ihandle *ih, int lin, int col);

**ih**: identifier of the element that activated the event.\
**lin**: new focus line (1-based).\
**col**: new focus column (1-based).

**SORT_CB**: Action generated when the user clicks a column header with SORTABLE=YES.

    int function(Ihandle *ih, int col);

**ih**: identifier of the element that activated the event.\
**col**: column number (1-based).

**Returns:** IUP_IGNORE to suppress the sort operation.

**VALUECHANGED_CB**: Called after a cell value was changed.

    int function(Ihandle *ih, int lin, int col);

**ih**: identifier of the element that activated the event.\
**lin**: line number (1-based).\
**col**: column number (1-based).

**EDITBEGIN_CB**: Called when a cell is about to enter edit mode.

    int function(Ihandle *ih, int lin, int col);

**ih**: identifier of the element that activated the event.\
**lin**: line number (1-based).\
**col**: column number (1-based).

**Returns:** IUP_IGNORE to block editing.

**EDITEND_CB**: Called when cell editing ends.

    int function(Ihandle *ih, int lin, int col, char *new_value, int apply);

**ih**: identifier of the element that activated the event.\
**lin**: line number (1-based).\
**col**: column number (1-based).\
**new_value**: the new text value entered by the user.\
**apply**: 1 if the user accepted the edit (Enter), 0 if cancelled (Escape).

**Returns:** IUP_IGNORE to reject the edit and keep the old value.

**EDITION_CB**: Called while the user is editing a cell, on each text change.

    int function(Ihandle *ih, int lin, int col, char *new_text);

**ih**: identifier of the element that activated the event.\
**lin**: line number (1-based).\
**col**: column number (1-based).\
**new_text**: the current text in the editor.

**Returns:** IUP_IGNORE to prevent the text update.

**VALUE_CB**: Called to retrieve the cell value in virtual mode.

    char* function(Ihandle *ih, int lin, int col);

**ih**: identifier of the element that activated the event.\
**lin**: line number (1-based).\
**col**: column number (1-based).

**Returns:** the string value to display in the cell.

**IMAGE_CB**: Called to retrieve the cell image in virtual mode.

    char* function(Ihandle *ih, int lin, int col);

**ih**: identifier of the element that activated the event.\
**lin**: line number (1-based).\
**col**: column number (1-based).

**Returns:** the image name to display in the cell.

>
>
> ------------------------------------------------------------------------

[MAP_CB](../call/iup_map_cb.md), [UNMAP_CB](../call/iup_unmap_cb.md), [DESTROY_CB](../call/iup_destroy_cb.md), [GETFOCUS_CB](../call/iup_getfocus_cb.md), [KILLFOCUS_CB](../call/iup_killfocus_cb.md), [ENTERWINDOW_CB](../call/iup_enterwindow_cb.md), [LEAVEWINDOW_CB](../call/iup_leavewindow_cb.md), [K_ANY](../call/iup_k_any.md), [HELP_CB](../call/iup_help_cb.md): All common callbacks are supported.

### Notes

Column and line indices are 1-based throughout the IupTable API.

In virtual mode, the table does not store cell values internally.
Instead, it calls VALUE_CB (and optionally IMAGE_CB) to retrieve the data to display.
This is efficient for very large datasets where storing all values would be impractical.
The application is responsible for maintaining the actual data.

When SORTABLE=YES, clicking a column header fires SORT_CB.
The application must perform the actual sorting and update the table.
Sort direction arrows are shown in the column header.

In GTK uses GtkTreeView with GtkListStore, in GTK 4 uses GtkColumnView, in Windows uses ListView in Report mode, in WinUI uses a custom XAML Grid layout with ListView for data rows, in macOS uses NSTableView, in Qt uses QTableWidget, in EFL uses a custom table drawn on Elm_Scroller, and in Motif uses a custom table drawn on XmDrawingArea.

### See Also

[IupMatrix](../ctrl/iup_matrix.md), [IupList](iup_list.md)
