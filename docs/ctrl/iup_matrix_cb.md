## IupMatrix Callbacks

### Interaction

**ACTION_CB**: Action generated when a keyboard event occurs.

    int function(Ihandle *ih, int key, int lin, int col, int edition, char* value);

**ih**: identifier of the element that activated the event.\
**key**: Identifier of the typed key. Please refer to the [Keyboard Codes](../attrib/iup_keyboard_codes.md) table for a list of possible values.\
**lin**, **col**: Coordinates of the selected cell.\
**edition**: 1 if the cell is in edition mode, and 0 if it is not.\
**value**: When EDITMODE=NO is the cell current value, but if the type key is a valid character then contains a string with that character.
When EDITMODE=Yes depends on the editing field type. If a dropdown, then it is an empty string ("").
If a text, and the type key is a valid character then it is the future value of the text field, if not a valid character then it is the cell current value.
Notice that this value can be NULL if the cell does not have a value and the key pressed is not a character.

**Returns:** IUP_DEFAULT validates the key, IUP_IGNORE ignores the key, IUP_CONTINUE forwards the key to IUPs conventional processing, or the identifier of the key to be treated by the matrix.

**CLICK_CB**: Action generated when any mouse button is pressed over a cell.
This callback is always called after other callbacks.
When  EDITHIDEONFOCUS=NO and editing is on going the callback EDITCLICK_CB with the same parameters will also be called right before this one.

    int function(Ihandle *ih, int lin, int col, char *status);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the cell where the mouse button was pressed.\
**status**: Status of the mouse buttons and some keyboard keys at the moment the event is generated.
The same macros used for [BUTTON_CB](../call/iup_button_cb.md) can be used for this status.

**Returns:** To avoid the display update return IUP_IGNORE.

**COLRESIZE_CB**: Action generated when a column is interactively resized.

    int function(Ihandle *ih, int col);

**ih**: identifier of the element that activated the event.\
**col**: Column that had its size changed.\

**RELEASE_CB**: Action generated when any mouse button is released over a cell.
This callback is always called after other callbacks.
When  EDITHIDEONFOCUS=NO and editing is on going the callback EDITRELEASE_CB with the same parameters will also be called right before this one.

    int function(Ihandle *ih, int lin, int col, char *status);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the cell where the mouse button was pressed.\
**status**: Status of the mouse buttons and some keyboard keys at the moment the event is generated.
The same macros used for [BUTTON_CB](../call/iup_button_cb.md) can be used for this status.

**Returns:** To avoid the display update return IUP_IGNORE.

**RESIZEMATRIX_CB**: Action generated after the element size has been updated but before the cells have been actually refreshed. Can be used to resize columns or lines when the matrix is resized by setting a column or line size to null and setting FITTOSIZE to COLUMNS or LINES.

    int function(Ihandle *ih, int width, int height);

**ih**: identifier of the element that activated the event.\
**width**: the width of the internal element size in pixels not considering the BORDER size (client size)\
**height**: the height of the internal element size in pixels not considering the BORDER size (client size)

**MOUSEMOVE_CB**: Action generated to notify the application that the mouse has moved over the matrix.
When  EDITHIDEONFOCUS=NO and editing is on going the callback EDITMOUSEMOVE_CB with the same parameters will also be called right before this one.

    int function(Ihandle *ih, int lin, int col);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the cell that the mouse cursor is currently on.

**ENTERITEM_CB**: Action generated when a matrix cell is selected, becoming the current cell.
Also called when matrix is getting focus.
Also called when focus is changed because lines or columns were added or removed.

    int function(Ihandle *ih, int lin, int col);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the selected cell.

**LEAVEITEM_CB**: Action generated when a cell is no longer the current cell.
Also called when the matrix is losing focus.

    int function(Ihandle *ih, int lin, int col);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the cell which is no longer the current cell.

**Returns:** IUP_IGNORE prevents the current cell from changing, but this will not work when the matrix is losing focus.  If you try to move to beyond matrix borders the cell will lose focus and then get it again, so leaveitem_cb and enteritem_cb will be called.

**SCROLLTOP_CB**: Action generated when the matrix is scrolled with the scrollbars or with the keyboard.
Can be used together with the ORIGIN and ORIGINOFFSET attributes to synchronize the movement of two or more matrices.

    int function(Ihandle *ih, int lin, int col);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the cell currently in the top-left corner of the matrix.

### Drawing

**BGCOLOR_CB** - Action generated to retrieve the background color of a cell when it needs to be redrawn.

    int function(Ihandle *ih, int lin, int col, int *red, int *green, int *blue);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the cell.\
**red**, **green**, **blue**: the cell background color.

**Returns:** If IUP_IGNORE, the values are ignored and the attribute defined background color will be used.
If returns IUP_DEFAULT the returned values will be used as the background color.

**FGCOLOR_CB** - Action generated to retrieve the foreground color of a cell when it needs to be redrawn.

    int function(Ihandle *ih, int lin, int col, int *red, int *green, int *blue);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the cell.\
red, green, blue: the cell foreground color.

**Returns:** If IUP_IGNORE, the values are ignored and the attribute defined foreground color will be used.
If returns IUP_DEFAULT the returned values will be used as the foreground color.

**FONT_CB**: Action generated to retrieve the font of a cell.
Called both for common cells and for line and column titles.

    char* function(Ihandle* ih, int lin, int col);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the cell.

**Returns:** Must return a font or NULL to use the attribute defined font.

**TYPE_CB**: Action generated to retrieve the type of a cell value.
Called both for common cells and for line and column titles.

    char* function(Ihandle* ih, int lin, int col);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the cell.

**Returns:** Must return "TEXT", "COLOR", "FILL" or "IMAGE".

**DRAW_CB**: Action generated before a cell is drawn. Allows to draw a custom cell contents.
The clipping is set for the bounding rectangle.
The callback is called after the cell background has been filled with the background color.
The focus feedback area is not included in the decoration size.

    int function(Ihandle *ih, int lin, int col, int x1, int x2, int y1, int y2);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the current cell.\
**x1**, **x2**, **y1**, **y2**: Bounding rectangle of the current cell in pixels, excluding the decorations.

Use [IupDraw](../func/iup_draw.md) functions to draw the cell contents.

**Returns:** If IUP_IGNORE the normal text drawing will take place.

**DROPCHECK_CB**: Action generated before the current cell is redrawn to determine if a dropdown/popup menu feedback or a toggle should be shown.
If this action is not registered, no feedback will be shown.
If the callback is defined and return IUP_DEFAULT for a cell, to show the dropdown/popup menu the user can simply do a single click in the drop feedback area of that cell.

    int function(Ihandle *ih, int lin, int col);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the cell.

**Returns:** IUP_DEFAULT will show a drop feedback, IUP_CONTINUE will show and enable the toggle button, or IUP_IGNORE to draw nothing.

**TRANSLATEVALUE_CB**: Action generated to translate the value of a cell during display and size computation.
Called both for common cells and for line and column titles.

    char* function(Ihandle* ih, int lin, int col, char* value);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the cell.\
value: original cell value

**Returns:** the string to be drawn.

### Editing (not called if READONLY=Yes) TOGGLEVALUE_CB: Action generated when a toggle button is pressed.

    int function(Ihandle *ih, int lin, int col, int status);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the cell where the mouse button was pressed.\
**status**: Value of the toggle. Can be 1 or 0.

**VALUECHANGED_CB**: Called after the value was interactively changed by the user or after a group of values where programmatically changed in a single operation.
When it was interactively changed the temporary attribute CELL_EDITED will be set to Yes during the callback.

    int function(Ihandle *ih);

**ih**: identifier of the element that activated the event.

**DROP_CB**: Action generated before the current cell enters edition mode to determine if a text field or a dropdown list will be shown.
It is called after EDITION_CB. If this action is not registered, a text field will be shown.
Its return determines what type of element will be used in the edition mode.
If the selected type is a dropdown, the values appearing in the dropdown must be fulfilled in this callback, just like elements are added to any list (the drop parameter is the handle of the dropdown list to be shown).
You should also set the lists current value ("VALUE"), the default is always "1".
The previously cell value can be verified from the given drop Ihandle via the "PREVIOUSVALUE" attribute.

    int function(Ihandle *ih, Ihandle *drop, int lin, int col);

**ih**: identifier of the element that activated the event.\
**drop**: Identifier of the dropdown list which will be shown to the user.\
**lin**, **col**: Coordinates of the current cell.

**Returns:** IUP_IGNORE to show a text-edition field, or IUP_DEFAULT to show a dropdown field.

**MENUDROP_CB**: Action generated before the current cell enters edition mode to determine if a popup menu will be shown instead of a text field or a dropdown.
If this action is registered and retunr IUP_DEFAULT the DROP_CB callback is not called, and the popup menu is shown.
Like DROP_CB, it is called after EDITION_CB.
The values appearing as menu items in the popup menu must be fulfilled in this callback, like elements are added to a list (the drop parameter is the handle of the popup menu to be shown, but the actual menu items will be added later by the internal processing).
You could also set the "VALUE" attribute that will add a mark to the menu item with the same number.
If IMAGEid is set then an IMAGE attribute will be set at the correspondent menu item.
The previously cell value can be verified from the given drop Ihandle via the "PREVIOUSVALUE" attribute.

    int function(Ihandle *ih, Ihandle *drop, int lin, int col);

**ih**: identifier of the element that activated the event.\
**drop**: Identifier of the popup menu which will be shown to the user.\
**lin**, **col**: Coordinates of the current cell.

**Returns:** IUP_IGNORE to not show the menu for the given cell, DROP_CB will then be called.

**DROPSELECT_CB**: Action generated when an element in the dropdown list or the popup menu is selected.
For the dropdown, if returns IUP_CONTINUE the value is accepted as a new value and the matrix leaves edition mode, else the item is selected and editing remains.
For the popup menu the returned value is ignored.

    int function(Ihandle *ih, int lin, int col, Ihandle *drop, char *t, int i, int v);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the current cell.\
**drop**: Identifier of the dropdown list or the popup menu shown to the user.\
**t**: Text of the item whose state was changed.\
**i**: Number of the item whose state was changed.\
**v**: Indicates if item was selected or unselected (1 or 0). Always 1 for the popup menu.

**EDITION_CB**: Action generated when the current cell enters or leaves the edition mode.
Not called if READONLY=YES.

    int function(Ihandle *ih, int lin, int col, int mode, int update); 

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the current cell.\
**mode**: 1 if the cell has entered the edition mode, or 0 if the cell has left the edition mode.\
**update**: used when mode=0 to identify if the value will be updated when the callback returns with IUP_DEFAULT.

**Returns:** can be IUP_DEFAULT, IUP_IGNORE or IUP_CONTINUE.

If the callback does not exists the cell can always be edited and the new value is always accepted.

When editing is started, **mode**=1 and **update**=0.
Editing is allowed if the callback returns IUP_DEFAULT, so to make the cell read-only return IUP_IGNORE.

When editing ends, **mode**=0 and **update** can be 0 or 1.
The new value is accepted only if the callback returns IUP_DEFAULT.
The VALUE attribute when consulted inside the callback returns the new value that will be updated to the cell.
**update**=0 only when the user cancel the editing by pressing the **Esc** key.
If the callback returns IUP_CONTINUE the edit mode is ended and the new value will not be updated, so the application can set a different value during the callback (useful to format the new value).
If the callback returns IUP_IGNORE the editing is not ended, with several exceptions: the **Esc** key was used; the matrix size, scroll or visibility was changed during edition mode; a click in another cell; or the edit control loses its focus.

This callback is also called when the user press **Del** to clear the cell contents or other multiple cell editing.
The callback will simply validate the operation for each cell been cleared by checking if the matrix is read-only or if the cell is read-only.
In this situation it is called with **mode**=1 and **update**=1.
When in normal mode (not callback mode) the new value can not be refused, but you can use the VALUE_EDIT_CB to reset a new value or use the VALUECHANGED_CB to check all the new values after they where changed.

### Callback Mode

**VALUE_CB**: Action generated to retrieve the value of a cell.
Called both for common cells and for line and column titles.

    char* function(Ihandle* ih, int lin, int col);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the cell.

**Returns:** the string to be drawn.

**IMPORTANT**: The existence of this callback defines the callback operation mode of the matrix when it is mapped.

**VALUE_EDIT_CB**: Action generated to notify the application that the value of a cell was changed.
Never called when READONLY=YES. This callback is usually set in callback mode, but also works in normal mode.
When in normal mode, it is called after the new value has been internally stored, so to refuse the new value simply reset the cell to the desired value.
When it was interactively changed the temporary attribute CELL_EDITED will be set to Yes during the callback.

    int function(Ihandle *ih, int lin, int col, char* newval);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the cell.\
**newval**: String containing the new cell value

**IMPORTANT**: if VALUE_CB is defined and VALUE_EDIT_CB is not defined when the matrix is mapped it will be read-only.

**MARK_CB**: Action generated to retrieve the selection state of a cell.
Called only for common cells, only when MARKMODE=CELL and only in callback mode.

    int function(Ihandle* ih, int lin, int col);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the cell.

**Returns:** the selection state (marked=1, not marked 0).
If not defined the attribute "**MARK*L*:*C***" will be returned.

**MARKEDIT_CB**: Action generated to notify the application that the selection state of a cell was changed.
Since it is a notification, it cannot refuse the mark modification.
Called only for common cells, only when MARKMODE=CELL and only in callback mode.

    int function(Ihandle *ih, int lin, int col, int marked);

**ih**: identifier of the element that activated the event.\
**lin**, **col**: Coordinates of the cell.\
**marked**: selection state (marked=1, not marked 0).

If not defined the attribute "**MARK*L*:*C***" will be updated.
So if you define the **MARKEDIT_CB** the "**MARK*L*:*C***" will NOT be updated and the callback **MARK_CB** must return the selection state of the cell.
If you do not want to implement the **MARK_CB** callback then set the "**MARK*L*:*C***" attribute inside the **MARKEDIT_CB** callback.

------------------------------------------------------------------------

[MAP_CB](../call/iup_map_cb.md), [UNMAP_CB](../call/iup_unmap_cb.md), [DESTROY_CB](../call/iup_destroy_cb.md), [GETFOCUS_CB](../call/iup_getfocus_cb.md), [KILLFOCUS_CB](../call/iup_killfocus_cb.md), [ENTERWINDOW_CB](../call/iup_enterwindow_cb.md), [LEAVEWINDOW_CB](../call/iup_leavewindow_cb.md), [K_ANY](../call/iup_k_any.md), [HELP_CB](../call/iup_help_cb.md): All common callbacks are supported.

The **IupCanvas** callbacks [ACTION](../call/iup_action.md), [SCROLL_CB](../call/iup_scroll_cb.md), [KEYPRESS_CB](../call/iup_keypress_cb.md), [MOTION_CB](../call/iup_motion_cb.md), [FOCUS_CB](../elem/iup_canvas.md), [RESIZE_CB](../call/iup_resize_cb.md) and [BUTTON_CB](../call/iup_button_cb.md) can be changed but you should save and call the original definition from inside your own callback, or the matrix will not correctly work.

Use [IupConvertXYToPos](../func/iup_convertxytopos.md) to convert (x,y) coordinates in the cell position, then use [IupTextConvertPosToLinCol](../elem/iup_text.md) to convert pos into (lin,col), or use the formula "pos=lin*(NUMCOL+1) + col".
Here lin and col starts at 0, pos starts at 0.

See [IupCanvas](../elem/iup_canvas.md).
